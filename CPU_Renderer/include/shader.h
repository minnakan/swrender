#pragma once
#include "geometry.h"
#include "rasterizer.h"
#include "tgaimage.h"
#include <vector>

class Shader
{
  public:
  virtual TGAColor shade(const Rasterizer::Fragment& frag) const = 0;
  virtual ~Shader() = default;
};

class BlinnPhongShader : public Shader
{
  public:
  BlinnPhongShader(Vec3f lightDir, Vec3f cameraPos, const TGAImage* diffuse = nullptr,
                   const TGAImage* specular = nullptr, const TGAImage* normalMap = nullptr,
                   const std::vector<float>* shadowMap = nullptr, Mat4 lightMat = Mat4::identity(),
                   int shadowW = 0, int shadowH = 0, float ambient = 0.1f, float shininess = 32.f);

  TGAColor shade(const Rasterizer::Fragment& frag) const override;

  private:
  Vec3f                    lightDir_;
  Vec3f                    cameraPos_;
  const TGAImage*          diffuse_;
  const TGAImage*          specular_;
  const TGAImage*          normalMap_;
  const std::vector<float>* shadowMap_;
  Mat4                     lightMat_;
  int                      shadowW_;
  int                      shadowH_;
  float                    ambient_;
  float                    shininess_;
};
