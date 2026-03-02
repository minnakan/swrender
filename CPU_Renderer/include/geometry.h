#pragma once
#include <cmath>
#include <array>

// ─── Vec2 ────────────────────────────────────────────────────────────────────

template <typename T> struct Vec2
{
  T x{}, y{};

  Vec2() = default;
  Vec2(T x, T y) : x(x), y(y)
  {
  }

  Vec2 operator+(const Vec2& o) const
  {
    return {x + o.x, y + o.y};
  }
  Vec2 operator-(const Vec2& o) const
  {
    return {x - o.x, y - o.y};
  }
  Vec2 operator*(T s) const
  {
    return {x * s, y * s};
  }
};

using Vec2f = Vec2<float>;

// ─── Vec3 ────────────────────────────────────────────────────────────────────

template <typename T> struct Vec3
{
  T x{}, y{}, z{};

  Vec3() = default;
  Vec3(T x, T y, T z) : x(x), y(y), z(z)
  {
  }

  T& operator[](int i)
  {
    return i == 0 ? x : (i == 1 ? y : z);
  }
  const T& operator[](int i) const
  {
    return i == 0 ? x : (i == 1 ? y : z);
  }

  Vec3 operator+(const Vec3& o) const
  {
    return {x + o.x, y + o.y, z + o.z};
  }
  Vec3 operator-(const Vec3& o) const
  {
    return {x - o.x, y - o.y, z - o.z};
  }
  Vec3 operator*(T s) const
  {
    return {x * s, y * s, z * s};
  }
  Vec3 operator/(T s) const
  {
    return {x / s, y / s, z / s};
  }
  Vec3 operator-() const
  {
    return {-x, -y, -z};
  }
  Vec3& operator+=(const Vec3& o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  Vec3& operator-=(const Vec3& o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }

  T dot(const Vec3& o) const
  {
    return x * o.x + y * o.y + z * o.z;
  }
  Vec3 cross(const Vec3& o) const
  {
    return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
  }
  float norm() const
  {
    return std::sqrt((float)(x * x + y * y + z * z));
  }
  Vec3 normalized() const
  {
    return *this / (T)norm();
  }
};

template <typename T> Vec3<T> operator*(T s, const Vec3<T>& v)
{
  return v * s;
}
template <typename T> T dot(const Vec3<T>& a, const Vec3<T>& b)
{
  return a.dot(b);
}
template <typename T> Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b)
{
  return a.cross(b);
}

using Vec3f = Vec3<float>;

// ─── Vec4 ────────────────────────────────────────────────────────────────────

template <typename T> struct Vec4
{
  T x{}, y{}, z{}, w{};

  Vec4() = default;
  Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w)
  {
  }
  Vec4(Vec3<T> v, T w) : x(v.x), y(v.y), z(v.z), w(w)
  {
  }

  T& operator[](int i)
  {
    return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }
  const T& operator[](int i) const
  {
    return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }

  Vec4 operator+(const Vec4& o) const
  {
    return {x + o.x, y + o.y, z + o.z, w + o.w};
  }
  Vec4 operator-(const Vec4& o) const
  {
    return {x - o.x, y - o.y, z - o.z, w - o.w};
  }
  Vec4 operator*(T s) const
  {
    return {x * s, y * s, z * s, w * s};
  }
  Vec4 operator/(T s) const
  {
    return {x / s, y / s, z / s, w / s};
  }
  Vec4 operator-() const
  {
    return {-x, -y, -z, -w};
  }
  Vec4& operator+=(const Vec4& o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    w += o.w;
    return *this;
  }
  Vec4& operator-=(const Vec4& o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    w -= o.w;
    return *this;
  }

  T dot(const Vec4& o) const
  {
    return x * o.x + y * o.y + z * o.z + w * o.w;
  }
  float norm() const
  {
    return std::sqrt((float)(x * x + y * y + z * z + w * w));
  }
  Vec4 normalized() const
  {
    return *this / (T)norm();
  }

  Vec3<T> xyz() const
  {
    return {x, y, z};
  }
};

template <typename T> Vec4<T> operator*(T s, const Vec4<T>& v)
{
  return v * s;
}
template <typename T> T dot(const Vec4<T>& a, const Vec4<T>& b)
{
  return a.dot(b);
}

using Vec4f = Vec4<float>;

// ─── Mat ─────────────────────────────────────────────────────────────────────

template <int N> struct Mat
{
  float data[N][N] = {};

  static Mat identity()
  {
    Mat m;
    for (int i = 0; i < N; i++)
      m.data[i][i] = 1.f;
    return m;
  }

  float* operator[](int i)
  {
    return data[i];
  }
  const float* operator[](int i) const
  {
    return data[i];
  }

  Mat operator*(const Mat& o) const
  {
    Mat result;
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++)
        for (int k = 0; k < N; k++)
          result[i][j] += data[i][k] * o[k][j];
    return result;
  }

  // multiply by a column vector — works with any Vec that has operator[]
  template <typename Vec> Vec operator*(const Vec& v) const
  {
    Vec result{};
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++)
        result[i] += data[i][j] * v[j];
    return result;
  }
};

using Mat3 = Mat<3>;
using Mat4 = Mat<4>;
