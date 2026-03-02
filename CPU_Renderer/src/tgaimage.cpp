#include "tgaimage.h"
#include <cstring>
#include <fstream>
#include <cstdint>

TGAImage::TGAImage(const int w, const int h, const int bpp) : w(w), h(h), bpp(bpp), data(w * h * bpp, 0)
{
}

TGAImage::TGAImage(const std::string& filename)
{
  std::ifstream f(filename, std::ios::binary);
  if (!f)
    return;

  std::uint8_t header[18] = {};
  f.read(reinterpret_cast<char*>(header), 18);

  w   = header[12] | (header[13] << 8);
  h   = header[14] | (header[15] << 8);
  bpp = header[16] / 8;
  f.ignore(header[0]); // skip image ID field

  int nPixels = w * h;
  data.resize(nPixels * bpp);

  int imgType = header[2];
  if (imgType == 2)
  { // uncompressed true-color
    f.read(reinterpret_cast<char*>(data.data()), (std::streamsize)data.size());
  }
  else if (imgType == 10)
  { // RLE true-color
    int pixIdx = 0;
    while (pixIdx < nPixels)
    {
      std::uint8_t chunk = 0;
      f.read(reinterpret_cast<char*>(&chunk), 1);
      int count = (chunk & 0x7F) + 1;
      if (chunk & 0x80)
      { // run-length packet
        std::uint8_t pixel[4] = {};
        f.read(reinterpret_cast<char*>(pixel), bpp);
        for (int i = 0; i < count; i++)
          memcpy(data.data() + pixIdx++ * bpp, pixel, bpp);
      }
      else
      { // raw packet
        f.read(reinterpret_cast<char*>(data.data() + pixIdx * bpp), count * bpp);
        pixIdx += count;
      }
    }
  }

  // TGA is bottom-origin by default; flip if the top-left origin flag is set
  bool topOrigin = (header[17] & 0x20) != 0;
  if (topOrigin)
    flip_vertically();
}

TGAColor TGAImage::get(int x, int y) const
{
  TGAColor c;
  if (data.empty() || x < 0 || y < 0 || x >= w || y >= h)
    return c;
  memcpy(c.bgra, data.data() + (x + y * w) * bpp, bpp);
  return c;
}

void TGAImage::clear(TGAColor c)
{
  for (int i = 0; i < w * h; i++)
    memcpy(data.data() + i * bpp, c.bgra, bpp);
}

void TGAImage::set(int x, int y, const TGAColor& c)
{
  if (!data.size() || x < 0 || y < 0 || x >= w || y >= h)
    return;
  memcpy(data.data() + (x + y * w) * bpp, c.bgra, bpp);
}

void TGAImage::flip_vertically()
{
  for (int i = 0; i < w; i++)
    for (int j = 0; j < h / 2; j++)
      for (int b = 0; b < bpp; b++)
        std::swap(data[(i + j * w) * bpp + b], data[(i + (h - 1 - j) * w) * bpp + b]);
}

int TGAImage::width() const
{
  return w;
}

int TGAImage::height() const
{
  return h;
}

void TGAImage::write(const std::string& filename) const
{
  std::ofstream f(filename, std::ios::binary);
  if (!f)
    return;

  std::uint8_t header[18] = {};
  header[2]               = 2; // uncompressed true-color
  header[12]              = w & 0xFF;
  header[13]              = (w >> 8) & 0xFF;
  header[14]              = h & 0xFF;
  header[15]              = (h >> 8) & 0xFF;
  header[16]              = bpp * 8;

  f.write(reinterpret_cast<const char*>(header), 18);
  f.write(reinterpret_cast<const char*>(data.data()), (std::streamsize)data.size());
}
