#include "model.h"
#include <cstdio>
#include <fstream>
#include <sstream>

Model::Model(const std::string& filename)
{
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line))
  {
    std::istringstream ss(line);
    std::string token;
    ss >> token;

    if (token == "v")
    {
      Vec3f v;
      ss >> v.x >> v.y >> v.z;
      verts_.push_back(v);
    }
    else if (token == "vn")
    {
      Vec3f n;
      ss >> n.x >> n.y >> n.z;
      normals_.push_back(n);
    }
    else if (token == "vt")
    {
      Vec2f uv;
      ss >> uv.x >> uv.y;
      uvs_.push_back(uv);
    }
    else if (token == "f")
    {
      std::array<FaceVertex, 3> face;
      for (int i = 0; i < 3; i++)
      {
        std::string t;
        ss >> t;
        int v, vt, vn;
        if (std::sscanf(t.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3)
          face[i] = {v - 1, vt - 1, vn - 1};
        else if (std::sscanf(t.c_str(), "%d//%d", &v, &vn) == 2)
          face[i] = {v - 1, -1, vn - 1};
        else if (std::sscanf(t.c_str(), "%d/%d", &v, &vt) == 2)
          face[i] = {v - 1, vt - 1, -1};
        else
          face[i] = {std::stoi(t) - 1, -1, -1};
      }
      faces_.push_back(face);
    }
  }
}

int Model::nfaces() const
{
  return static_cast<int>(faces_.size());
}

Vec3f Model::vert(int face, int vert) const
{
  return verts_[faces_[face][vert].v];
}

Vec3f Model::normal(int face, int vert) const
{
  int idx = faces_[face][vert].vn;
  return idx >= 0 ? normals_[idx] : Vec3f{0.f, 0.f, 1.f};
}

Vec2f Model::uv(int face, int vert) const
{
  int idx = faces_[face][vert].vt;
  return idx >= 0 ? uvs_[idx] : Vec2f{0.f, 0.f};
}
