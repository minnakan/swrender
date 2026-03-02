// CPU_Renderer.cpp : Defines the entry point for the application.
//

#include "camera.h"
#include "model.h"
#include "pipeline.h"
#include "rasterizer.h"
#include "shader.h"
#include "tgaimage.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#include <array>
#include <functional>
#include <limits>
#include <omp.h>
#include <random>
#include <vector>

namespace
{

struct RenderOutput
{
  TGAImage fb;
  int threads;
};

void display(std::function<RenderOutput()> rasterize, Camera& camera, int renderW, int renderH, int initWinW,
             int initWinH)
{
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  SDL_Window* window = SDL_CreateWindow("CPU Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, initWinW,
                                        initWinH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_Texture* fbTexture =
      SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, renderW, renderH);

  TTF_Font* font = TTF_OpenFont("resources/font.ttf", 18);

  auto uploadImages = [&](RenderOutput output)
  {
    output.fb.flip_vertically();
    SDL_UpdateTexture(fbTexture, nullptr, output.fb.rawData(), renderW * 3);
  };

  bool showStats = false;
  double lastRenderMs = 0.0;
  int lastThreads = 0;

  std::function<void()> render = [&]()
  {
    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);

    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);

    SDL_Rect full = {0, 0, winW, winH};
    SDL_RenderCopy(sdlRenderer, fbTexture, nullptr, &full);

    if (font)
    {
      SDL_Color white = {255, 255, 255, 255};

      if (showStats)
      {
        constexpr int pad = 8;
        constexpr int lineH = 22;

        char lines[2][48];
        std::snprintf(lines[0], sizeof(lines[0]), "Render:  %.1f ms", lastRenderMs);
        std::snprintf(lines[1], sizeof(lines[1]), "Threads: %d", lastThreads);

        constexpr int panelW = 190;
        int panelH = 2 * lineH + 2 * pad;
        int panelX = winW - panelW - pad;
        int panelY = pad;

        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 180);
        SDL_Rect bg = {panelX, panelY, panelW, panelH};
        SDL_RenderFillRect(sdlRenderer, &bg);
        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_NONE);

        for (int i = 0; i < 2; i++)
        {
          SDL_Surface* surf = TTF_RenderText_Blended(font, lines[i], white);
          SDL_Texture* tex = SDL_CreateTextureFromSurface(sdlRenderer, surf);
          SDL_Rect dst = {panelX + pad, panelY + pad + i * lineH, surf->w, surf->h};
          SDL_RenderCopy(sdlRenderer, tex, nullptr, &dst);
          SDL_FreeSurface(surf);
          SDL_DestroyTexture(tex);
        }
      }
    }

    SDL_RenderPresent(sdlRenderer);
  };

  {
    auto out = rasterize();
    lastThreads = out.threads;
    uploadImages(std::move(out));
  }
  render();

  // fires from inside Windows' modal resize loop so we redraw during border drag
  SDL_EventFilter resizeWatch = [](void* data, SDL_Event* e) -> int
  {
    if (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      (*static_cast<std::function<void()>*>(data))();
    return 0;
  };
  SDL_AddEventWatch(resizeWatch, &render);

  constexpr float orbitSpeed = 0.01f;
  constexpr float panSpeed = 0.05f;
  constexpr float zoomSpeed = 0.2f;

  const Camera initialCamera = camera;

  bool mouseDown = false;
  int lastMouseX = 0, lastMouseY = 0;

  auto redraw = [&]()
  {
    Uint64 t0 = SDL_GetPerformanceCounter();
    auto out = rasterize();
    Uint64 t1 = SDL_GetPerformanceCounter();
    lastRenderMs = (double)(t1 - t0) / SDL_GetPerformanceFrequency() * 1000.0;
    lastThreads = out.threads;
    uploadImages(std::move(out));
    render();
  };

  SDL_Event event;
  bool running = true;
  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        running = false;
        break;

      // ── mouse orbit ───────────────────────────────────────────────────────
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouseDown = true;
          lastMouseX = event.button.x;
          lastMouseY = event.button.y;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
          mouseDown = false;
        break;

      case SDL_MOUSEMOTION:
        if (mouseDown)
        {
          camera.orbit((event.motion.x - lastMouseX) * orbitSpeed, (event.motion.y - lastMouseY) * orbitSpeed);
          lastMouseX = event.motion.x;
          lastMouseY = event.motion.y;
          redraw();
        }
        break;

      // ── scroll / +- zoom ─────────────────────────────────────────────────
      case SDL_MOUSEWHEEL:
        camera.zoom(event.wheel.y * zoomSpeed);
        redraw();
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        // stats panel toggle
        case SDLK_BACKQUOTE:
          showStats = !showStats;
          render();
          break;

        // reset
        case SDLK_r:
          camera = initialCamera;
          redraw();
          break;

        // zoom
        case SDLK_EQUALS:
        case SDLK_PLUS:
          camera.zoom(zoomSpeed);
          redraw();
          break;
        case SDLK_MINUS:
          camera.zoom(-zoomSpeed);
          redraw();
          break;

        // arrow keys — shift held: pan, plain: orbit
        case SDLK_LEFT:
          if (event.key.keysym.mod & KMOD_SHIFT)
            camera.pan(-panSpeed, 0.f);
          else
            camera.orbit(-orbitSpeed * 5.f, 0.f);
          redraw();
          break;

        case SDLK_RIGHT:
          if (event.key.keysym.mod & KMOD_SHIFT)
            camera.pan(panSpeed, 0.f);
          else
            camera.orbit(orbitSpeed * 5.f, 0.f);
          redraw();
          break;

        case SDLK_UP:
          if (event.key.keysym.mod & KMOD_SHIFT)
            camera.pan(0.f, panSpeed);
          else
            camera.orbit(0.f, -orbitSpeed * 5.f);
          redraw();
          break;

        case SDLK_DOWN:
          if (event.key.keysym.mod & KMOD_SHIFT)
            camera.pan(0.f, -panSpeed);
          else
            camera.orbit(0.f, orbitSpeed * 5.f);
          redraw();
          break;
        }
        break;
      }
    }
  }

  SDL_DelEventWatch(resizeWatch, &render);
  if (font)
    TTF_CloseFont(font);
  SDL_DestroyTexture(fbTexture);
  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
}
} // namespace

int main()
{
  constexpr int renderWidth = 1024;
  constexpr int renderHeight = 1024;
  constexpr int winWidth = 800;
  constexpr int winHeight = 800;

  Model    model("resources/obj/diablo3_pose/diablo3_pose.obj");
  TGAImage diffuse("resources/obj/diablo3_pose/diablo3_pose_diffuse.tga");
  TGAImage specular("resources/obj/diablo3_pose/diablo3_pose_spec.tga");
  TGAImage normalMap("resources/obj/diablo3_pose/diablo3_pose_nm_tangent.tga");

  //Model    model("resources/obj/african_head/african_head.obj");
  //TGAImage diffuse("resources/obj/african_head/african_head_diffuse.tga");
  //TGAImage specular("resources/obj/african_head/african_head_spec.tga");
  //TGAImage normalMap("resources/obj/african_head/african_head_nm_tangent.tga");

  Model    floor("resources/obj/floor.obj");
  TGAImage floorDiffuse("resources/obj/floor_diffuse.tga");
  TGAImage floorSpecular("resources/obj/floor_spec.tga");
  TGAImage floorNormalMap("resources/obj/floor_nm_tangent.tga");
  floor.transform[0][0] = 8.f;
  floor.transform[2][2] = 8.f;
  floor.transform[1][3] = -0.25f; // drop floor below model feet

  Camera camera;
  camera.aspect = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);

  const Mat4  vp        = viewport(renderWidth, renderHeight);
  const Vec3f lightPos  = {5.f, 1.5f, 4.f};
  const Mat4  lightView = lookAt(lightPos, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f});
  const Mat4  lightProj = ortho(-4.f, 4.f, -4.f, 4.f, 0.1f, 20.f);
  const Mat4  lightVP   = viewport(renderWidth, renderHeight);
  const Mat4  lightMat  = lightVP * lightProj * lightView;

  auto rasterize = [&]() -> RenderOutput
  {
    TGAImage framebuffer(renderWidth, renderHeight, TGAImage::RGB);
    framebuffer.clear({60, 60, 60, 255}); // dark gray background
    std::vector<float> zbuffer(renderWidth * renderHeight, -std::numeric_limits<float>::max());

    // ── shadow pass (all models) ───────────────────────────────────────────────
    std::vector<float> shadowMap(renderWidth * renderHeight, -std::numeric_limits<float>::max());

    auto shadowPass = [&](const Model& m)
    {
      for (int f = 0; f < m.nfaces(); f++)
      {
        PipelineTriangle shadowTri = {{
            PipelineVertex{toClip(m.vert(f, 0), lightProj, lightView, m.transform)},
            PipelineVertex{toClip(m.vert(f, 1), lightProj, lightView, m.transform)},
            PipelineVertex{toClip(m.vert(f, 2), lightProj, lightView, m.transform)},
        }};
        for (const auto& tri : clip(shadowTri))
        {
          Rasterizer::Vertex sv0 = {toScreen(tri[0].pos, lightVP), 1.f / tri[0].pos.w};
          Rasterizer::Vertex sv1 = {toScreen(tri[1].pos, lightVP), 1.f / tri[1].pos.w};
          Rasterizer::Vertex sv2 = {toScreen(tri[2].pos, lightVP), 1.f / tri[2].pos.w};
          Rasterizer::depthOnly(sv0, sv1, sv2, shadowMap, renderWidth, renderHeight);
        }
      }
    };

    shadowPass(model);
    shadowPass(floor);

    // ── main pass ─────────────────────────────────────────────────────────────
    Mat4        view     = camera.view();
    Mat4        proj     = camera.projection();
    const Vec3f lightDir = lightPos.normalized();

    // G-buffers for SSAO — declared here so mainPass can write into them
    std::vector<Vec3f> gbufPos(renderWidth * renderHeight);
    std::vector<Vec3f> gbufNormal(renderWidth * renderHeight);

    auto mainPass = [&](const Model& m, const BlinnPhongShader& sh)
    {
      for (int f = 0; f < m.nfaces(); f++)
      {
        Vec3f normalA = (m.transform * Vec4f(m.normal(f, 0), 0.f)).xyz().normalized();
        Vec3f normalB = (m.transform * Vec4f(m.normal(f, 1), 0.f)).xyz().normalized();
        Vec3f normalC = (m.transform * Vec4f(m.normal(f, 2), 0.f)).xyz().normalized();

        Vec3f worldPosA = (m.transform * Vec4f(m.vert(f, 0), 1.f)).xyz();
        Vec3f worldPosB = (m.transform * Vec4f(m.vert(f, 1), 1.f)).xyz();
        Vec3f worldPosC = (m.transform * Vec4f(m.vert(f, 2), 1.f)).xyz();

        Vec3f edge1   = worldPosB - worldPosA;
        Vec3f edge2   = worldPosC - worldPosA;
        Vec2f duv1    = m.uv(f, 1) - m.uv(f, 0);
        Vec2f duv2    = m.uv(f, 2) - m.uv(f, 0);
        float det     = duv1.x * duv2.y - duv1.y * duv2.x;
        Vec3f tangent = std::abs(det) > 1e-6f ? ((edge1 * duv2.y - edge2 * duv1.y) * (1.f / det)).normalized()
                                              : cross(edge1, edge2).normalized();

        PipelineTriangle clipTri = {{
            {toClip(m.vert(f, 0), proj, view, m.transform), normalA, m.uv(f, 0), worldPosA, tangent},
            {toClip(m.vert(f, 1), proj, view, m.transform), normalB, m.uv(f, 1), worldPosB, tangent},
            {toClip(m.vert(f, 2), proj, view, m.transform), normalC, m.uv(f, 2), worldPosC, tangent},
        }};

        for (const auto& tri : clip(clipTri))
        {
          Rasterizer::Vertex sv0 = {toScreen(tri[0].pos, vp), 1.f / tri[0].pos.w, tri[0].normal, tri[0].uv, tri[0].worldPos, tri[0].tangent};
          Rasterizer::Vertex sv1 = {toScreen(tri[1].pos, vp), 1.f / tri[1].pos.w, tri[1].normal, tri[1].uv, tri[1].worldPos, tri[1].tangent};
          Rasterizer::Vertex sv2 = {toScreen(tri[2].pos, vp), 1.f / tri[2].pos.w, tri[2].normal, tri[2].uv, tri[2].worldPos, tri[2].tangent};
          Rasterizer::triangle(sv0, sv1, sv2, zbuffer, framebuffer, sh, &gbufPos, &gbufNormal);
        }
      }
    };

    int activeThreads = 1;

    const BlinnPhongShader modelShader{lightDir, camera.eye, &diffuse,      &specular,      &normalMap,      &shadowMap, lightMat, renderWidth, renderHeight};
    const BlinnPhongShader floorShader{lightDir, camera.eye, &floorDiffuse, &floorSpecular, &floorNormalMap, &shadowMap, lightMat, renderWidth, renderHeight};

    mainPass(model, modelShader);
    mainPass(floor, floorShader);

    // ── SSAO pass ─────────────────────────────────────────────────────────────
    // Hemisphere sample kernel (generated once)
    static const auto ssaoKernel = []()
    {
      std::mt19937                          rng(42);
      std::uniform_real_distribution<float> d01(0.f, 1.f);
      std::uniform_real_distribution<float> dN(-1.f, 1.f);
      std::array<Vec3f, 16>                 k;
      for (int i = 0; i < 16; ++i)
      {
        Vec3f s     = Vec3f{dN(rng), dN(rng), d01(rng)}.normalized() * d01(rng);
        float scale = float(i) / 16.f;
        k[i]        = s * (0.1f + 0.9f * scale * scale);
      }
      return k;
    }();

    // Random rotation vectors tiled over screen (4×4 noise)
    static const auto ssaoNoise = []()
    {
      std::mt19937                          rng(1337);
      std::uniform_real_distribution<float> dN(-1.f, 1.f);
      std::array<Vec3f, 16>                 n;
      for (auto& v : n)
        v = Vec3f{dN(rng), dN(rng), 0.f}.normalized();
      return n;
    }();

    constexpr float aoRadius = 0.5f;
    constexpr float aoBias   = 0.02f;

    std::vector<float> aoMap(renderWidth * renderHeight, 1.f);

    for (int y = 0; y < renderHeight; ++y)
    {
      for (int x = 0; x < renderWidth; ++x)
      {
        int   idx    = x + y * renderWidth;
        Vec3f fragPos = gbufPos[idx];
        Vec3f fragN   = gbufNormal[idx];

        if (fragN.norm() < 0.5f)
          continue; // background pixel

        fragN = fragN.normalized();

        // Build a TBN using a random rotation vector to orient the hemisphere
        const Vec3f& rnv = ssaoNoise[(x % 4) + (y % 4) * 4];
        Vec3f        T   = (rnv - fragN * dot(fragN, rnv)).normalized();
        Vec3f        B   = cross(fragN, T);

        float occlusion = 0.f;
        for (const auto& k : ssaoKernel)
        {
          Vec3f samplePos = fragPos + (T * k.x + B * k.y + fragN * k.z) * aoRadius;

          // Project to screen to look up actual geometry at that pixel
          Vec4f sc = proj * view * Vec4f(samplePos, 1.f);
          if (sc.w <= 0.f)
            continue;
          Vec3f ss = toScreen(sc, vp);
          int   sx = (int)(ss.x + 0.5f);
          int   sy = (int)(ss.y + 0.5f);
          if (sx < 0 || sx >= renderWidth || sy < 0 || sy >= renderHeight)
            continue;

          Vec3f occPos = gbufPos[sx + sy * renderWidth];
          if (occPos.norm() < 1e-6f)
            continue; // background

          // Compare depth in view space: less-negative z = closer to camera
          float sampleVZ   = (view * Vec4f(samplePos, 1.f)).z;
          float occluderVZ = (view * Vec4f(occPos, 1.f)).z;

          // Weight by proximity so distant geometry doesn't contribute
          float dist      = (occPos - fragPos).norm();
          float rangeCheck = std::max(0.f, 1.f - dist / (aoRadius * 2.f));

          if (occluderVZ > sampleVZ + aoBias)
            occlusion += rangeCheck;
        }

        aoMap[idx] = 1.f - std::min(1.f, occlusion / (float)ssaoKernel.size());
      }
    }

    // ── Box blur (5×5) ────────────────────────────────────────────────────────
    std::vector<float> aoBlurred(renderWidth * renderHeight, 1.f);
    constexpr int blurR = 2;
    for (int y = 0; y < renderHeight; ++y)
    {
      for (int x = 0; x < renderWidth; ++x)
      {
        float sum = 0.f;
        int   cnt = 0;
        for (int dy = -blurR; dy <= blurR; ++dy)
          for (int dx = -blurR; dx <= blurR; ++dx)
          {
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= renderWidth || ny < 0 || ny >= renderHeight)
              continue;
            sum += aoMap[nx + ny * renderWidth];
            ++cnt;
          }
        aoBlurred[x + y * renderWidth] = cnt > 0 ? sum / cnt : 1.f;
      }
    }

    // ── Debug writes (first frame only) ──────────────────────────────────────
    static bool debugSaved = false;
    if (!debugSaved)
    {
      // AO map
      TGAImage aoImage(renderWidth, renderHeight, TGAImage::RGB);
      for (int y = 0; y < renderHeight; ++y)
        for (int x = 0; x < renderWidth; ++x)
        {
          auto v = static_cast<std::uint8_t>(aoBlurred[x + y * renderWidth] * 255.f);
          aoImage.set(x, y, {v, v, v, 255});
        }
      aoImage.flip_vertically();
      aoImage.write("ao_debug.tga");

      // Framebuffer before AO
      TGAImage before = framebuffer;
      before.flip_vertically();
      before.write("before_ao.tga");
    }

    // ── Apply AO to framebuffer ───────────────────────────────────────────────
    for (int y = 0; y < renderHeight; ++y)
      for (int x = 0; x < renderWidth; ++x)
      {
        float    ao = aoBlurred[x + y * renderWidth];
        TGAColor c  = framebuffer.get(x, y);
        framebuffer.set(x, y, {static_cast<std::uint8_t>(c[0] * ao), static_cast<std::uint8_t>(c[1] * ao),
                                static_cast<std::uint8_t>(c[2] * ao), 255});
      }

    if (!debugSaved)
    {
      debugSaved = true;
      TGAImage after = framebuffer;
      after.flip_vertically();
      after.write("after_ao.tga");
    }

    return {std::move(framebuffer), activeThreads};
  };

  display(rasterize, camera, renderWidth, renderHeight, winWidth, winHeight);
  return 0;
}
