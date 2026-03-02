#pragma once
#include "geometry.h"
#include <array>
#include <vector>

// Vertex at the clip/raster boundary.
// Carries clip-space position alongside per-vertex attributes so the clipper
// can interpolate them correctly when splitting triangles at frustum edges.
struct PipelineVertex
{
  Vec4f pos;      // clip-space position
  Vec3f normal;   // world-space
  Vec2f uv;
  Vec3f worldPos; // world-space
  Vec3f tangent;  // world-space
};

using PipelineTriangle = std::array<PipelineVertex, 3>;

// ─── coordinate transforms ────────────────────────────────────────────────────

Vec4f toClip(Vec3f v, const Mat4& proj, const Mat4& view, const Mat4& model);
Vec3f toScreen(Vec4f clip, const Mat4& vp);
Mat4  viewport(int w, int h);
Mat4  lookAt(Vec3f eye, Vec3f center, Vec3f up);
Mat4  ortho(float l, float r, float b, float t, float nearZ, float farZ);

// ─── clipping ────────────────────────────────────────────────────────────────

// Clips a triangle against all 6 frustum planes.
// Returns 0 or more triangles — 0 means fully culled.
std::vector<PipelineTriangle> clip(const PipelineTriangle& tri);
