#pragma once
#include "geometry.h"

class Camera
{
  public:
  Vec3f eye = {0.f, 0.f, 3.f};
  Vec3f center = {0.f, 0.f, 0.f};
  Vec3f up = {0.f, 1.f, 0.f};
  float fovY = 45.f * 3.14159265f / 180.f;
  float aspect = 1.f;
  float nearZ = 0.1f;
  float farZ = 10.f;

  Mat4 view() const;
  Mat4 projection() const;

  void orbit(float yaw, float pitch);
  void pan(float dx, float dy);
  void zoom(float delta);
};
