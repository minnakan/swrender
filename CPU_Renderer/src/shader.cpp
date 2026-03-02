#include "shader.h"
#include <algorithm>
#include <cmath>

BlinnPhongShader::BlinnPhongShader(Vec3f lightDir, Vec3f cameraPos, const TGAImage* diffuse,
                                   const TGAImage* specular, const TGAImage* normalMap,
                                   const std::vector<float>* shadowMap, Mat4 lightMat, int shadowW,
                                   int shadowH, float ambient, float shininess)
    : lightDir_(lightDir.normalized()), cameraPos_(cameraPos), diffuse_(diffuse), specular_(specular),
      normalMap_(normalMap), shadowMap_(shadowMap), lightMat_(lightMat), shadowW_(shadowW),
      shadowH_(shadowH), ambient_(ambient), shininess_(shininess)
{
}

TGAColor BlinnPhongShader::shade(const Rasterizer::Fragment& frag) const
{
  int tx = diffuse_ ? (int)(frag.uv.x * diffuse_->width())  % diffuse_->width()  : 0;
  int ty = diffuse_ ? (int)(frag.uv.y * diffuse_->height()) % diffuse_->height() : 0;

  // ── normal ────────────────────────────────────────────────────────────────
  Vec3f N = frag.normal.normalized();
  if (normalMap_)
  {
    // Sample tangent-space normal and decode from [0,1] → [-1,1]
    TGAColor nm = normalMap_->get(tx, ty);
    Vec3f tangentNormal = {nm[2] / 255.f * 2.f - 1.f,  // r → x
                           nm[1] / 255.f * 2.f - 1.f,  // g → y
                           nm[0] / 255.f * 2.f - 1.f}; // b → z

    // Gram-Schmidt orthogonalise T against N, then build TBN
    Vec3f T = (frag.tangent - N * dot(N, frag.tangent)).normalized();
    Vec3f B = cross(N, T);
    // Transform tangent-space normal to world space
    N = (T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z).normalized();
  }

  // ── lighting ──────────────────────────────────────────────────────────────
  Vec3f V = (cameraPos_ - frag.worldPos).normalized();
  Vec3f H = (lightDir_ + V).normalized();

  float specCoeff = specular_ ? specular_->get(tx, ty)[0] / 255.f : 0.5f;
  float diff      = std::max(0.f, dot(N, lightDir_));
  float spec      = std::pow(std::max(0.f, dot(N, H)), shininess_) * specCoeff;

  // ── shadow test ───────────────────────────────────────────────────────────
  float shadow = 1.f;
  if (shadowMap_)
  {
    Vec4f sc = lightMat_ * Vec4f(frag.worldPos, 1.f);
    int   sx = (int)(sc.x / sc.w);
    int   sy = (int)(sc.y / sc.w);
    float sz = sc.z / sc.w;
    if (sx >= 0 && sx < shadowW_ && sy >= 0 && sy < shadowH_)
    {
      constexpr float bias = 0.005f;
      if (sz < (*shadowMap_)[sx + sy * shadowW_] - bias)
        shadow = 0.f; // fully in shadow — only ambient contributes
    }
  }

  float intensity = std::min(1.f, ambient_ + (diff + spec) * shadow);

  if (diffuse_)
  {
    TGAColor c = diffuse_->get(tx, ty);
    return {static_cast<std::uint8_t>(c[0] * intensity),
            static_cast<std::uint8_t>(c[1] * intensity),
            static_cast<std::uint8_t>(c[2] * intensity), 255};
  }

  auto v = static_cast<std::uint8_t>(intensity * 255);
  return {v, v, v, 255};
}
