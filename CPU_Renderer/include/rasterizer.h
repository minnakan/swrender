#pragma once
#include "geometry.h"
#include "tgaimage.h"
#include <vector>

class Shader; // defined in shader.h

class Rasterizer
{
  public:
  struct Vertex
  {
    Vec3f pos;      // screen-space (x,y for coverage, z for depth test)
    float invW;     // 1/clip_w — enables perspective-correct interpolation
    Vec3f normal;   // world-space
    Vec2f uv;
    Vec3f worldPos; // world-space position
    Vec3f tangent;  // world-space
  };

  struct Fragment
  {
    Vec3f normal;   // perspective-correct interpolated, world-space
    Vec2f uv;       // perspective-correct interpolated
    Vec3f worldPos; // perspective-correct interpolated, world-space
    Vec3f tangent;  // perspective-correct interpolated, world-space
  };

  static void triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, std::vector<float>& zbuffer, TGAImage& fb,
                       const Shader& shader, std::vector<Vec3f>* gbufPos = nullptr,
                       std::vector<Vec3f>* gbufNormal = nullptr);
  static void depthOnly(const Vertex& v0, const Vertex& v1, const Vertex& v2, std::vector<float>& zbuffer, int width,
                        int height);

  private:
  Rasterizer() = delete;
};
