#pragma once
#include "geometry.h"
#include <array>
#include <string>
#include <vector>

class Model
{
  public:
  Model(const std::string& filename);

  Mat4 transform = Mat4::identity(); // model-to-world transform

  int nfaces() const;
  Vec3f vert(int face, int vert) const;
  Vec3f normal(int face, int vert) const;
  Vec2f uv(int face, int vert) const;

  private:
  struct FaceVertex
  {
    int v, vt, vn; // indices into verts_, uvs_, normals_ (-1 = absent)
  };

  std::vector<Vec3f> verts_;
  std::vector<Vec3f> normals_;
  std::vector<Vec2f> uvs_;
  std::vector<std::array<FaceVertex, 3>> faces_;
};
