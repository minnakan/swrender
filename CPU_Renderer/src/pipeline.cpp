#include "pipeline.h"
#include <functional>

// ─── coordinate transforms ────────────────────────────────────────────────────

Vec4f toClip(Vec3f v, const Mat4& proj, const Mat4& view, const Mat4& model)
{
  return proj * (view * (model * Vec4f(v, 1.f)));
}

Vec3f toScreen(Vec4f clip, const Mat4& vp)
{
  Vec3f ndc = clip.xyz() / clip.w; // perspective divide — clip → NDC
  return (vp * Vec4f(ndc, 1.f)).xyz();
}

Mat4 viewport(int w, int h)
{
  float hw = w * 0.5f;
  float hh = h * 0.5f;

  Mat4 m = Mat4::identity();
  m[0][0] = hw;
  m[0][3] = hw;
  m[1][1] = hh;
  m[1][3] = hh;
  m[2][2] = -0.5f;
  m[2][3] = 0.5f; // near → 1, far → 0
  return m;
}

Mat4 lookAt(Vec3f eye, Vec3f center, Vec3f up)
{
  Vec3f forward = (eye - center).normalized();
  Vec3f right   = cross(up, forward).normalized();
  Vec3f upOrtho = cross(forward, right);

  Mat4 m        = Mat4::identity();
  m[0][0]       = right.x;
  m[0][1]       = right.y;
  m[0][2]       = right.z;
  m[0][3]       = -dot(right, eye);
  m[1][0]       = upOrtho.x;
  m[1][1]       = upOrtho.y;
  m[1][2]       = upOrtho.z;
  m[1][3]       = -dot(upOrtho, eye);
  m[2][0]       = forward.x;
  m[2][1]       = forward.y;
  m[2][2]       = forward.z;
  m[2][3]       = -dot(forward, eye);
  return m;
}

Mat4 ortho(float l, float r, float b, float t, float nearZ, float farZ)
{
  Mat4 m    = Mat4::identity();
  m[0][0]   = 2.f / (r - l);
  m[0][3]   = -(r + l) / (r - l);
  m[1][1]   = 2.f / (t - b);
  m[1][3]   = -(t + b) / (t - b);
  m[2][2]   = 2.f / (nearZ - farZ);
  m[2][3]   = (nearZ + farZ) / (nearZ - farZ);
  return m;
}

// ─── clipping ─────────────────────────────────────────────────────────────────

static std::vector<PipelineVertex> clipAgainstPlane(const std::vector<PipelineVertex>& poly,
                                                    const std::function<float(const PipelineVertex&)>& inside)
{
  std::vector<PipelineVertex> result;
  int n = (int)poly.size();

  for (int i = 0; i < n; i++)
  {
    const PipelineVertex& a = poly[i];
    const PipelineVertex& b = poly[(i + 1) % n];
    float dA = inside(a);
    float dB = inside(b);

    if (dA >= 0)
      result.push_back(a);

    if ((dA >= 0) != (dB >= 0))
    {
      float t = dA / (dA - dB);
      PipelineVertex interp;
      interp.pos      = a.pos      + (b.pos      - a.pos)      * t;
      interp.normal   = (a.normal  + (b.normal   - a.normal)   * t).normalized();
      interp.uv       = a.uv       + (b.uv       - a.uv)       * t;
      interp.worldPos = a.worldPos + (b.worldPos - a.worldPos) * t;
      interp.tangent  = a.tangent  + (b.tangent  - a.tangent)  * t;
      result.push_back(interp);
    }
  }

  return result;
}

std::vector<PipelineTriangle> clip(const PipelineTriangle& tri)
{
  static const std::array<std::function<float(const PipelineVertex&)>, 6> planes = {{
      [](const PipelineVertex& v) { return v.pos.w + v.pos.x; }, // left:   x >= -w
      [](const PipelineVertex& v) { return v.pos.w - v.pos.x; }, // right:  x <=  w
      [](const PipelineVertex& v) { return v.pos.w + v.pos.y; }, // bottom: y >= -w
      [](const PipelineVertex& v) { return v.pos.w - v.pos.y; }, // top:    y <=  w
      [](const PipelineVertex& v) { return v.pos.w + v.pos.z; }, // near:   z >= -w
      [](const PipelineVertex& v) { return v.pos.w - v.pos.z; }, // far:    z <=  w
  }};

  std::vector<PipelineVertex> poly = {tri[0], tri[1], tri[2]};

  for (const auto& plane : planes)
  {
    poly = clipAgainstPlane(poly, plane);
    if (poly.empty())
      return {};
  }

  // re-triangulate clipped polygon as a fan from poly[0]
  std::vector<PipelineTriangle> result;
  for (int i = 1; i + 1 < (int)poly.size(); i++)
    result.push_back({poly[0], poly[i], poly[i + 1]});
  return result;
}
