#pragma once

//
// ;scalar
//

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

inline f32
toBilateral(f32 r)
{
    return (r * 2.0f) - 1.0f;
}

inline f32
toUnilateral(f32 r)
{
    return (r * 0.5f) + 0.5f;
}

inline f32
lerp(f32 a, f32 t, f32 b)
{
    f32 result;
    result = a + t*(b - a);
    return result;
}

inline f32
unlerp_or_zero(f32 a, f32 v, f32 b)
{
  f32 range = (b - a);
  f32 result = (range != 0.0f) ? ((v - a) / range) : 0.0f;
  return result;
}

inline b32
inRange(f32 value, f32 min, f32 max)
{
  return ((min <= value) && (value <= max));
}

inline b32
inRange(i32 value, i32 min, i32 max)
{
    return ((min <= value) && (value <= max));
}

#define inRange01(v) inRange(v, 0.f, 1.f)

inline f32
clamp(f32 value, f32 min, f32 max)
{
    kvAssert(max >= min);
    if (value > max)
    {
        return max;
    }
    else if (value < min)
    {
        return min;
    }
    return value;
}

inline f32
clamp01(f32 r)
{
    return clamp(r, 0, 1);
}

inline f32
clampMin(f32 r, f32 min)
{
    f32 result = maximum(r, min);
    return result;
}

inline i32
clampMin(i32 r, i32 min)
{
    i32 result = maximum(r, min);
    return result;
}

inline f32
clampMax(f32 r, f32 max)
{
    f32 result = minimum(r, max);
    return result;
}

inline i32
clampMax(i32 r, i32 max)
{
    i32 result = minimum(r, max);
    return result;
}

//
// scalar]
//

//
// ;v2
//

union v2 {
  struct {
    f32 x, y;
  };
  f32 E[2];
  f32 v[2];
};

inline v2
toV2(f32 x, f32 y)
{
    v2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline v2
v2i(i32 x, i32 y)
{
    return toV2((f32)x, (f32)y);
}

inline v2
v2All(f32 c)
{
    return toV2(c, c);
}

inline b32
operator==(v2 u, v2 v)
{
    b32 result;
    result = (u.x == v.x) && (u.y == v.y);
    return result;
}

inline b32
operator!=(v2 u, v2 v)
{
    b32 result;
    result = (u.x != v.x) || (u.y != v.y);
    return result;
}

inline v2
operator+(v2 u, v2 v)
{
    v2 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    return result;
}

inline v2 &
operator+=(v2 &v, v2 u)
{
    v = u + v;
    return v;
}

inline v2
operator-(v2 u, v2 v)
{
    v2 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    return result;
}

inline v2
operator-=(v2 &v, v2 u)
{
    v = v - u;
    return v;
}

inline v2
operator-(v2 v)
{
    v2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

inline v2
operator*(f32 c, v2 v)
{
    v2 result;
    result.x = c * v.x;
    result.y = c * v.y;
    return result;
}

inline v2
operator*(v2 v, f32 c)
{
    v2 result = c*v;
    return result;
}

inline v2 &
operator*=(v2 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v2
operator/(v2 v, f32 c)
{
    v2 result;
    result.x = v.x / c;
    result.y = v.y / c;
    return result;
}

inline f32
dot(v2 v, v2 u)
{
    f32 result = v.x*u.x + v.y*u.y;
    return result;
}

inline f32
lengthSq(v2 v)
{
    f32 result = square(v.x) + square(v.y);
    return result;
}

inline f32
length(v2 v)
{
    f32 result = squareRoot(lengthSq(v));
    return result;
}

inline f32
projectLen(v2 onto, v2 v)
{
    f32 innerProd = dot(onto, v);
    f32 result = (innerProd / length(onto));
    return result;
}

inline v2
project(v2 onto, v2 v)
{
    f32 innerProd = dot(onto, v);
    v2 result = (innerProd / lengthSq(onto)) * onto;
    return result;
}

inline v2
hadamard(v2 v, v2 u)
{
    v2 result;
    result.x = v.x*u.x;
    result.y = v.y*u.y;
    return result;
}

inline v2
normalize(v2 v)
{
    v2 result;
    f32 len = length(v);
    if (len == 0)
    {
        result = toV2(0,0);
    }
    else
    {
        result = v * (1.0f / len);
    }
    return result;
}

inline v2
noz(v2 v)  // normalize or zero
{
  f32 lsq = lengthSq(v);
  v2 result = {};
  if (lsq > square(.0001f)) {
    // prevent the result from getting too big
    result = v * 1.f / squareRoot(lsq);
  }
  return result;
}

inline v2
perp(v2 v)
{
    v2 result = {-v.y, v.x};
    return result;
}

inline v2
toBilateral(v2 v)
{
    v2 result = {toBilateral(v.x), toBilateral(v.y)};
    return result;
}

inline b32
inRange(v2 v, f32 min, f32 max)
{
    b32 result = (inRange(v.x, min, max) && inRange(v.y, min, max));
    return result;
}

//
// v2]
//

// ;v3

union v3 {
  struct {
    f32 x, y, z;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    v2 xy;
    f32 ignored;
  };
  f32 E[3];
  f32 v[3];
};

inline v3
toV3(f32 x, f32 y, f32 z)
{
    v3 result = { x, y, z };
    return result;
}

inline v3
v3i(i32 x, i32 y, i32 z)
{
    v3 result;
    result.x = (f32)x;
    result.y = (f32)y;
    result.z = (f32)z;
    return result;
}

inline v3
toV3(v2 xy, f32 z)
{
    v3 result;
    result.xy = xy;
    result.z = z;
    return result;
}

inline v3
v3All(f32 c)
{
    return toV3(c, c, c);
}

inline v3
v3z(f32 z)
{
    v3 result = {0,0,z};
    return result;
}

inline v3
v3xy(v2 v)
{
    v3 result = {v.x, v.y, 0};
    return result;
}

inline v3
operator-(v3 u, v3 v)
{
    v3 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    result.z = u.z - v.z;
    return result;
}

inline b32
operator<(v3 u, v3 v)
{
    b32 result = ((u.x < v.x) && (u.y < v.y) && (u.z < v.z));
    return result;
}

inline b32
operator<=(v3 u, v3 v)
{
    b32 result = ((u.x <= v.x) && (u.y <= v.y) && (u.z <= v.z));
    return result;
}

inline b32
operator>(v3 u, v3 v)
{
    b32 result = ((u.x > v.x) && (u.y > v.y) && (u.z > v.z));
    return result;
}

inline b32
operator>=(v3 u, v3 v)
{
    b32 result = ((u.x >= v.x) && (u.y >= v.y) && (u.z >= v.z));
    return result;
}

inline b32
operator==(v3 u, v3 v)
{
    b32 result;
    result = (u.x == v.x) && (u.y == v.y) && (u.z == v.z);
    return result;
}

inline b32
operator!=(v3 u, v3 v)
{
    b32 result;
    result = (u.x != v.x) || (u.y != v.y) || (u.z != v.z);
    return result;
}

inline v3
operator+(v3 u, v3 v)
{
    v3 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    result.z = u.z + v.z;
    return result;
}

inline v3 &
operator+=(v3 &v, v3 u)
{
    v = u + v;
    return v;
}

inline v3
operator-=(v3 &v, v3 u)
{
    v = v - u;
    return v;
}


inline v3
operator-(v3 v)
{
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

inline v3
operator*(f32 c, v3 v)
{
    v3 result;
    result.x = c * v.x;
    result.y = c * v.y;
    result.z = c * v.z;
    return result;
}

inline v3
operator*(v3 v, f32 c)
{
    v3 result = c*v;
    return result;
}

inline v3 &
operator*=(v3 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v3
operator/(v3 v, f32 c)
{
    v3 result;
    result.x = v.x / c;
    result.y = v.y / c;
    result.z = v.z / c;
    return result;
}

inline f32
dot(v3 v, v3 u)
{
    f32 result = v.x*u.x + v.y*u.y + v.z*u.z;
    return result;
}

inline v3
cross(v3 v, v3 u)
{
  return v3{v.y*u.z - v.z*u.y,
            v.z*u.x - v.x*u.z,
            v.x*u.y - v.y*u.x};
}

inline v3
hadamard(v3 v, v3 u)
{
    v3 result;
    result.x = v.x*u.x;
    result.y = v.y*u.y;
    result.z = v.z*u.z;
    return result;
}

inline f32
lengthSq(v3 v)
{
    f32 result = square(v.x) + square(v.y) + square(v.z);
    return result;
}

inline f32
length(v3 v)
{
    f32 result = squareRoot(lengthSq(v));
    return result;
}

inline v3
normalize(v3 v)
{
    f32 len = length(v);
    v3 result = v * (1.f / len);
    return result;
}

inline v3
noz(v3 v)  // normalize or zero
{
  f32 lsq = lengthSq(v);
  v3 result = {};
  if (lsq > square(.0001f)) {
    // prevent the result from getting too big
    result = v * 1.f / squareRoot(lsq);
  }
  return result;
}

inline v3
project(v3 onto, v3 v)
{
    f32 innerProd = dot(onto, v);
    v3 result = (innerProd / lengthSq(onto)) * onto;
    return result;
}

inline v3
toBilateral(v3 v)
{
    v3 result;
    result.x = toBilateral(v.x);
    result.y = toBilateral(v.y);
    result.z = toBilateral(v.z);
    return result;
}

// v3; //

// ;v4 //

union v4 {
  struct {
    f32 x, y, z, w;
  };
  struct {
    f32 r, g, b, a;
  };
  struct{
    f32 h;
    f32 s;
    f32 l;
    f32 __a;
  };
  struct {
    v3 rgb;
    f32 a_ignored;
  };
  struct {
    v3 xyz;
    f32 w_ignored;
  };
  struct {
    v2 xy;
    v2 yz_ignored;
  };
  f32 E[4];
  f32 v[4];
};

typedef v4 Vec4_f32;

inline v4
toV4(v3 xyz, f32 w)
{
    v4 result = { xyz.x, xyz.y, xyz.z , w };
    return result;
}

inline v4
hadamard(v4 u, v4 v)
{
    v4 result;
    result.x = u.x * v.x;
    result.y = u.y * v.y;
    result.z = u.z * v.z;
    result.w = u.w * v.w;
    return result;
}

inline v4
clampMax(v4 v, f32 max)
{
    v4 result = { clampMax(v.x, max),
                  clampMax(v.y, max),
                  clampMax(v.z, max),
                  clampMax(v.w, max) };
    return result;
}

inline v4
operator*(f32 c, v4 v)
{
    v4 result = {c * v.x, c * v.y, c * v.z, c * v.w};
    return result;
}

inline v4
operator*(v4 v, f32 c)
{
    v4 result = {c * v.x, c * v.y, c * v.z, c * v.w};
    return result;
}

inline v4 &
operator*=(v4 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v4
operator+(v4 u, v4 v)
{
    v4 result = {u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w};
    return result;
}

inline v4
operator-(v4 u, v4 v)
{
    v4 result = {u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w};
    return result;
}

inline v4
lerp(v4 a, f32 t, v4 b)
{
    v4 result;
    result = a + t*(b - a);
    return result;
}

inline b32
inRange(v4 v, f32 min, f32 max)
{
    b32 result = (inRange(v.x, min, max) &&
                  inRange(v.y, min, max) &&
                  inRange(v.z, min, max) &&
                  inRange(v.w, min, max));
    return result;
}

//
// v4]
//

//
// ;rect2
//

typedef v2 Vec2_f32;

union rect2 {
  struct {
    v2 min;
    v2 max;
  };
  struct{
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
  };
  struct{
    Vec2_f32 p0;
    Vec2_f32 p1;
  };
  Vec2_f32 p[2];
};

inline b32
inRectangle(rect2 rect, v2 point)
{
    return ((point.x >= rect.min.x) && (point.y >= rect.min.y)
            && (point.x < rect.max.x) && (point.y < rect.max.y));
}

inline rect2
rectRadius(v2 radius)
{
    return {-radius, radius};
}

inline v2
getRectDim(rect2 rect)
{
    return (rect.max - rect.min);
}

inline rect2
rectCenterRadius(v2 center, v2 radius)
{
    kvAssert((radius.x >= 0) && (radius.y >= 0));
    rect2 result;
    result.min = center - radius;
    result.max = center + radius;
    return result;
}

inline rect2
rectCenterDim(v2 center, v2 dim)
{
    rect2 result = rectCenterRadius(center, 0.5f*dim);
    return result;
}

inline rect2
rectMinDim(v2 min, v2 dim)
{
  rect2 out = rect2{.min=min, .max=min+dim};
  return out;
}

inline rect2
intersect(rect2 a, rect2 b)
{
    rect2 result;
    result.min.x = maximum(a.min.x, b.min.x);
    result.min.y = maximum(a.min.y, b.min.y);
    result.max.x = minimum(a.max.x, b.max.x);
    result.max.y = minimum(a.max.y, b.max.y);
    return result;
}

//
// rect2]
//

//
// ;rect3
//

struct Rect3
{
    v3 min;
    v3 max;
};

inline b32
verifyRect(Rect3 rect)
{
    b32 result = ((rect.min.x <= rect.max.x)
                  && (rect.min.y <= rect.max.y)
                  && (rect.min.z <= rect.max.z));
    return result;
}

inline Rect3
rectRadius(v3 radius)
{
    return {-radius, radius};
}

inline Rect3
rectCenterRadius(v3 center, v3 radius)
{
    kvAssert((radius.x >= 0) && (radius.y >= 0) && (radius.z >= 0));
    Rect3 result;
    result.min = center - radius;
    result.max = center + radius;
    return result;
}

inline b32
inRectangle(Rect3 rect, v3 p)
{
    b32 result = ((p.x >= rect.min.x)
                  && (p.y >= rect.min.y)
                  && (p.z >= rect.min.z)
                  && (p.x < rect.max.x)
                  && (p.y < rect.max.y)
                  && (p.z < rect.max.z));
    return result;
}

inline Rect3
rectCenterDim(v3 center, v3 dim)
{
    Rect3 result = rectCenterRadius(center, 0.5f*dim);
    return result;
}

inline Rect3
rectDim(v3 dim)
{
    Rect3 result = rectCenterDim(v3All(0), dim);
    return result;
}

inline Rect3
minkowskiGrow(Rect3 a, Rect3 b)
{
    Rect3 result;
    v3 bDiameter = b.max - b.min;
    v3 bRadius = 0.5f * bDiameter;
    result.min = a.min - bRadius;
    result.max = a.max + bRadius;
    return result;
}

inline v3
getRectCenter(Rect3 rect)
{
    v3 result;
    result = 0.5f*(rect.min + rect.max);
    return result;
}

inline v3
getRectRadius(Rect3 rect)
{
    v3 result;
    result = 0.5f * (rect.max - rect.min);
    return result;
}

inline b32
rectOverlap(Rect3 a, Rect3 b)
{
    b32 separate = ((a.max.x <= b.min.x) || (a.min.x >= b.max.x)
                    || (a.max.y <= b.min.y) || (a.min.y >= b.max.y)
                    || (a.max.z <= b.min.z) || (a.min.z >= b.max.z));
    return !separate;
}

inline b32
rectInside(Rect3 in, Rect3 out)
{
    b32 result = ((in.min >= out.min) && (in.max <= out.max));
    return result;
}

inline Rect3
addRadiusTo(Rect3 a, v3 radius)
{
    Rect3 result = Rect3{a.min - radius, a.max + radius};
    return result;
}

inline v3
getBarycentricCoordinate(Rect3 rect, v3 pos)
{
    v3 result;
    v3 dim = rect.max - rect.min;
    result.x = ((pos.x - rect.min.x) / dim.x);
    result.y = ((pos.y - rect.min.y) / dim.y);
    result.z = ((pos.z - rect.min.z) / dim.z);
    return result;
}

//
// ]rect3
//
