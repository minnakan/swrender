// Taken from https://github.com/ssloy/tinyrenderer/
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct TGAColor
{
  std::uint8_t bgra[4] = {0, 0, 0, 0};
  std::uint8_t& operator[](const int i)
  {
    return bgra[i];
  }
  const std::uint8_t& operator[](const int i) const
  {
    return bgra[i];
  }
};

struct TGAImage
{
  enum Format
  {
    RGB = 3,
  };
  TGAImage() = default;
  TGAImage(const int w, const int h, const int bpp);
  TGAImage(const std::string& filename); // load from file
  void flip_vertically();
  void clear(TGAColor c);
  void set(const int x, const int y, const TGAColor& c);
  TGAColor get(int x, int y) const;
  int width() const;
  int height() const;
  void write(const std::string& filename) const;
  const std::uint8_t* rawData() const
  {
    return data.data();
  }

  private:
  int w = 0, h = 0;
  std::uint8_t bpp = 0;
  std::vector<std::uint8_t> data = {};
};
