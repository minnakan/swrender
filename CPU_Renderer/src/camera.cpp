#include "camera.h"
#include "pipeline.h"
#include <cmath>

Mat4 Camera::view() const
{
  return lookAt(eye, center, up);
}

Mat4 Camera::projection() const
{
  float f = 1.f / std::tan(fovY * 0.5f);

  Mat4 m; // zero-initialized
  m[0][0] = f / aspect;
  m[1][1] = f;
  m[2][2] = (farZ + nearZ) / (nearZ - farZ);
  m[2][3] = 2.f * farZ * nearZ / (nearZ - farZ);
  m[3][2] = -1.f;
  return m;
}

void Camera::orbit(float yaw, float pitch)
{
  Vec3f offset = eye - center;

  // yaw: rotate offset around world Y axis
  float cy = std::cos(yaw), sy = std::sin(yaw);
  offset = {cy * offset.x + sy * offset.z, offset.y, -sy * offset.x + cy * offset.z};

  // pitch: rotate offset around the camera's right axis (Rodrigues' formula)
  Vec3f right = cross(up, offset.normalized()).normalized();
  float cp = std::cos(pitch), sp = std::sin(pitch);
  offset = offset * cp + cross(right, offset) * sp + right * dot(right, offset) * (1.f - cp);

  eye = center + offset;
}

void Camera::pan(float dx, float dy)
{
  Vec3f forward = (eye - center).normalized();
  Vec3f right = cross(up, forward).normalized();
  Vec3f localUp = cross(forward, right);
  Vec3f delta = right * dx + localUp * dy;
  eye += delta;
  center += delta;
}

void Camera::zoom(float delta)
{
  Vec3f forward = (center - eye).normalized();
  eye += forward * delta;
}
