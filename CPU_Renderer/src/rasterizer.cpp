#include "rasterizer.h"
#include "shader.h"
#include <algorithm>
#include <limits>

void Rasterizer::depthOnly(const Vertex& v0, const Vertex& v1, const Vertex& v2, std::vector<float>& zbuffer,
                           int width, int height)
{
  int minX = std::max(0, (int)std::min({v0.pos.x, v1.pos.x, v2.pos.x}));
  int minY = std::max(0, (int)std::min({v0.pos.y, v1.pos.y, v2.pos.y}));
  int maxX = std::min(width - 1, (int)std::max({v0.pos.x, v1.pos.x, v2.pos.x}));
  int maxY = std::min(height - 1, (int)std::max({v0.pos.y, v1.pos.y, v2.pos.y}));

  float area = (v1.pos.x - v0.pos.x) * (v2.pos.y - v0.pos.y) - (v1.pos.y - v0.pos.y) * (v2.pos.x - v0.pos.x);
  if (area <= 0)
    return;

  float invArea = 1.f / area;

  auto edge = [](const Vec3f& a, const Vec3f& b, float px, float py)
  { return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x); };

  for (int y = minY; y <= maxY; y++)
  {
    for (int x = minX; x <= maxX; x++)
    {
      float e12 = edge(v1.pos, v2.pos, (float)x, (float)y);
      float e20 = edge(v2.pos, v0.pos, (float)x, (float)y);
      float e01 = edge(v0.pos, v1.pos, (float)x, (float)y);

      if (e12 < 0 || e20 < 0 || e01 < 0)
        continue;

      float l0 = e12 * invArea;
      float l1 = e20 * invArea;
      float l2 = e01 * invArea;

      float denom    = l0 * v0.invW + l1 * v1.invW + l2 * v2.invW;
      float z        = (l0 * v0.pos.z * v0.invW + l1 * v1.pos.z * v1.invW + l2 * v2.pos.z * v2.invW) / denom;
      int   idx      = x + y * width;
      if (z > zbuffer[idx])
        zbuffer[idx] = z;
    }
  }
}

void Rasterizer::triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, std::vector<float>& zbuffer,
                          TGAImage& fb, const Shader& shader, std::vector<Vec3f>* gbufPos,
                          std::vector<Vec3f>* gbufNormal)
{
  int minX = std::max(0, (int)std::min({v0.pos.x, v1.pos.x, v2.pos.x}));
  int minY = std::max(0, (int)std::min({v0.pos.y, v1.pos.y, v2.pos.y}));
  int maxX = std::min(fb.width() - 1, (int)std::max({v0.pos.x, v1.pos.x, v2.pos.x}));
  int maxY = std::min(fb.height() - 1, (int)std::max({v0.pos.y, v1.pos.y, v2.pos.y}));

  float area = (v1.pos.x - v0.pos.x) * (v2.pos.y - v0.pos.y) - (v1.pos.y - v0.pos.y) * (v2.pos.x - v0.pos.x);
  if (area <= 0)
    return; // back-facing or degenerate

  float invArea = 1.f / area;
  int width = fb.width();

  auto edge = [](const Vec3f& a, const Vec3f& b, float px, float py)
  { return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x); };

  for (int y = minY; y <= maxY; y++)
  {
    for (int x = minX; x <= maxX; x++)
    {
      // Edge functions — each gives the barycentric weight for the *opposite* vertex:
      //   e12 → weight for v0,  e20 → weight for v1,  e01 → weight for v2
      float e12 = edge(v1.pos, v2.pos, (float)x, (float)y);
      float e20 = edge(v2.pos, v0.pos, (float)x, (float)y);
      float e01 = edge(v0.pos, v1.pos, (float)x, (float)y);

      if (e12 < 0 || e20 < 0 || e01 < 0)
        continue;

      float l0 = e12 * invArea; // barycentric weight for v0
      float l1 = e20 * invArea; // barycentric weight for v1
      float l2 = e01 * invArea; // barycentric weight for v2

      // Perspective-correct denominator
      float denom = l0 * v0.invW + l1 * v1.invW + l2 * v2.invW;
      float invDenom = 1.f / denom;

      float z = (l0 * v0.pos.z * v0.invW + l1 * v1.pos.z * v1.invW + l2 * v2.pos.z * v2.invW) * invDenom;

      int idx = x + y * width;
      if (z <= zbuffer[idx])
        continue;

      zbuffer[idx] = z;

      Fragment frag;
      frag.normal   = (v0.normal   * (l0 * v0.invW) + v1.normal   * (l1 * v1.invW) + v2.normal   * (l2 * v2.invW)) * invDenom;
      frag.uv       = (v0.uv       * (l0 * v0.invW) + v1.uv       * (l1 * v1.invW) + v2.uv       * (l2 * v2.invW)) * invDenom;
      frag.worldPos = (v0.worldPos * (l0 * v0.invW) + v1.worldPos * (l1 * v1.invW) + v2.worldPos * (l2 * v2.invW)) * invDenom;
      frag.tangent  = (v0.tangent  * (l0 * v0.invW) + v1.tangent  * (l1 * v1.invW) + v2.tangent  * (l2 * v2.invW)) * invDenom;

      fb.set(x, y, shader.shade(frag));

      if (gbufPos)    (*gbufPos)[idx]    = frag.worldPos;
      if (gbufNormal) (*gbufNormal)[idx] = frag.normal.normalized();
    }
  }
}
