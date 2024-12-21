#pragma once
/* ;v2 */


myinline v1
v2::operator[](i32 index)
{
 return v[index];
}

inline v2 v2_all(v1 input)
{
 return v2{input, input};
}

myinline bool
operator==(v2 u, v2 v)
{
 bool result;
 result = (u.x == v.x) && (u.y == v.y);
 return result;
}

myinline v2
operator+(v2 u, v2 v)
{
 v2 result;
 result.x = u.x + v.x;
 result.y = u.y + v.y;
 return result;
}

myinline v1
lerp(v2 ab, v1 t)
{
 return lerp(ab[0], t, ab[1]);
}


myinline v2
operator-(v2 u, v2 v)
{
 v2 result;
 result.x = u.x - v.x;
 result.y = u.y - v.y;
 return result;
}

myinline v2
operator-=(v2 &v, v2 u)
{
 v = v - u;
 return v;
}

myinline v2
operator-(v2 v)
{
 v2 result;
 result.x = -v.x;
 result.y = -v.y;
 return result;
}

myinline v2
operator*(v1 c, v2 v)
{
 v2 result;
 result.x = c * v.x;
 result.y = c * v.y;
 return result;
}

myinline v2 operator*(v2 v, v1 c) { return c*v; }
inline v2 operator/(v2 v, v2 u) { return {v.x / u.x, v.y / u.y}; }
inline void operator*=(v2 &v, v1 c) { v = c*v; }
inline v2 operator/(v2 v, v1 c) { return v2{v.x / c, v.y / c}; }
inline v1 dot(v2 v, v2 u) { return v.x*u.x + v.y*u.y; }
inline v1 lensq(v2 v)    { return squared(v.x) + squared(v.y); }
inline v1 lengthof(v2 v) { return square_root(lensq(v)); }

inline v1
projectLen(v2 onto, v2 v)
{
 v1 innerProd = dot(onto, v);
 f32 result = (innerProd / lengthof(onto));
 return result;
}

inline v2
project_on(v2 onto, v2 v)
{
 f32 innerProd = dot(onto, v);
 v2 result = (innerProd / lensq(onto)) * onto;
 return result;
}

myinline v2
hadamard(v2 v, v2 u)
{
 v2 result;
 result.x = v.x*u.x;
 result.y = v.y*u.y;
 return result;
}

function v2
noz(v2 v)  // normalize or zero
{
 v1 lsq = lensq(v);
 v2 result = {};
 if (lsq > 1e-8)
 {
  result = v * 1.f / square_root(lsq);
 }
 return result;
}

myinline v2 perp(v2 v) { return v2{-v.y, v.x}; }

myinline v2 bilateral(v2 v)  { return v2{bilateral(v.x), bilateral(v.y)}; }

// ;v3



inline v3
absolute(v3 v){
 for_i32(index,0,3){ v[index] = absolute(v[index]); };
 return v;
}
myinline v3 V3(v2 xy)       { return v3{.xy=xy}; }
myinline v3 V3(v2 xy, v1 z) { return v3{.xy=xy, .xy_z=z}; }
myinline v3 yzx(v3 v) { return v3{v.y, v.z, v.x}; }
myinline v3 zxy(v3 v) { return v3{v.z, v.x, v.y}; }
inline v3 min(v3 a, v3 b){ return v3{min(a.x,b.x),min(a.y,b.y),min(a.z,b.z),}; }
inline v3 max(v3 a, v3 b){ return v3{max(a.x,b.x),max(a.y,b.y),max(a.z,b.z),}; }
inline v3 min(v3 a, v1 b){ return min(a,v3{repeat3(b)}); }
inline v3 max(v3 a, v1 b){ return max(a,v3{repeat3(b)}); }
inline v3
operator-(v3 u, v3 v){
 v3 result;
 result.x = u.x - v.x;
 result.y = u.y - v.y;
 result.z = u.z - v.z;
 return result;
}
inline b32
operator<(v3 u, v3 v){
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

inline bool
operator==(v3 u, v3 v)
{
 bool result;
 result = (u.x == v.x) && (u.y == v.y) && (u.z == v.z);
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

inline v3
operator-=(v3 &v, v3 u)
{
 v = v - u;
 return v;
}

myinline v3
operator-(v3 v)
{
 v3 result;
 result.x = -v.x;
 result.y = -v.y;
 result.z = -v.z;
 return result;
}
myinline v3
operator*(v1 c, v3 v)
{
 v.x *= c;
 v.y *= c;
 v.z *= c;
 return v;
}
myinline v3
operator*(v3 v, f32 c)
{
 v3 result = c*v;
 return result;
}

myinline v3 &
operator*=(v3 &v, f32 c)
{
 v = c * v;
 return v;
}

myinline v3
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


// todo: pick better name for this thing?
inline v1
cross2d(v2 u, v2 v)
{
 return u.x*v.y - u.y*v.x;
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
myinline v3 
operator*(v3 u, v3 v)
{
 return hadamard(u,v);
}
myinline v3 
operator/(v3 u, v3 v)
{
 return v3{u.x/v.x,
  u.y/v.y,
  u.z/v.z};
}

inline v1
lensq(v3 v)
{
 v1 result = squared(v.x) + squared(v.y) + squared(v.z);
 return result;
}

inline v1
lengthof(v3 v)
{
 v1 result = square_root(lensq(v));
 return result;
}

inline v3
project_on(v3 onto, v3 v)
{
 v1 innerProd = dot(onto, v);
 v3 result = (innerProd / lensq(onto)) * onto;
 return result;
}

inline v3
bilateral(v3 v)
{
 v3 result;
 result.x = bilateral(v.x);
 result.y = bilateral(v.y);
 result.z = bilateral(v.z);
 return result;
}

myinline v3
V3(v1 x, v1 y, v1 z)
{
 return v3{x, y, z};
}
myinline v4
V4(v1 x, v1 y, v1 z, v1 w)
{
 return v4{x, y, z, w};
}
myinline v4
vert4(v1 x, v1 y, v1 z)
{
 return v4{x, y, z, 1.f};
}
myinline v4
V4_symmetric(v1 x, v1 y)
{
 return v4{x,y,y,x};
}
myinline v4
V4_symmetric(v2 xy)
{
 return V4_symmetric(xy.x,xy.y);
}
myinline v4
V4(v3 xyz, v1 w)
{
 v4 v;
 v.xyz = xyz;
 v.w   = w;
 return v;
}
myinline v4
cast_V4(v3 xyz)
{
 v4 v = {};
 v.xyz = xyz;
 return v;
}

myinline v1 &
v4::operator[](i32 index)
{
 return v[index];
}

myinline v3
operator /(v1 n, v3 d)
{
 return V3(n/d.x, n/d.y, n/d.z);
}

typedef v4 Vec4_f32;

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
operator-(v4 v)
{
 v4 result = {-v.x, -v.y, -v.z, -v.w};
 return result;
}

inline v4
operator-(v4 u, v4 v)
{
 v4 result = {u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w};
 return result;
}

myinline b32 
almost_equal(v1 a, v1 b, v1 epsilon=1e-6)
{
 return absolute(a - b) < epsilon;
}

myinline b32 
almost_equal(v3 a, v3 b, v1 epsilon=1e-6) {
 for_i32(i,0,3) {
  if ( !almost_equal(a[i],b[i],epsilon) ) {
   return false;
  }
 }
 return true;
}


inline v4
lerp(v4 a, f32 t, v4 b)
{
 v4 result;
 result = a + t*(b - a);
 return result;
}

myinline void
operator+=(v3 &v, v3 u)
{
 v = u + v;
}

inline v2 &
operator+=(v2 &v, v2 u)
{
 v = u + v;
 return v;
}

inline v4 &
operator+=(v4 &v, v4 u)
{
 v = u + v;
 return v;
}

inline v3
noz(v3 v)  // normalize or zero
{
 v1 lsq = lensq(v);
 v3 result = {};
 if (lsq > 1e-8) 
 {
  // prevent the result from getting too big
  result = v * 1.f / square_root(lsq);
 }
 return result;
}

inline v1 
lensq(v4 v)
{
 return (v.x*v.x +
         v.y*v.y +
         v.z*v.z +
         v.w*v.w);
}

inline v4
noz(v4 v)
{
 v1 lsq = lensq(v);
 v4 result = {};
 if (lsq > squared(0.0001f)) 
 {
  // prevent the result from getting too big
  result = (1.f / square_root(lsq))*v;
 }
 return result;
}


// ;rect2

inline b32
contains(rect2 rect, v2 point)
{
 return ((point.x >= rect.x0) && (point.y >= rect.y0) &&
         (point.x <  rect.x1) && (point.y <  rect.y1));
}

inline rect2 rect2_radius(v2 radius) { return {-radius, radius}; }

inline v2 get_dim(rect2 rect) { return (rect.max - rect.min); }
inline v2 get_radius(rect2 rect) { return 0.5f*(rect.max - rect.min); }

inline rect2
rect2_center_radius(v2 center, v2 radius)
{
 ClampBot(radius.x,0);
 ClampBot(radius.y,0);
 rect2 result;
 result.min = center - radius;
 result.max = center + radius;
 return result;
}

inline rect2
rect2_center_dim(v2 center, v2 dim) {
 rect2 result = rect2_center_radius(center, 0.5f*dim);
 return result;
}

inline rect2
rect2_min_dim(v2 min, v2 dim)
{
 rect2 out = rect2{min, min+dim};
 return out;
}

inline rect2
rect2_min_max(v2 min, v2 max)
{
 rect2 result = rect2{min, max};
 return result;
}

inline rect2
intersect(rect2 a, rect2 b)
{
 rect2 result;
 result.min.x = macro_max(a.min.x, b.min.x);
 result.min.y = macro_max(a.min.y, b.min.y);
 result.max.x = minimum(a.max.x, b.max.x);
 result.max.y = minimum(a.max.y, b.max.y);
 return result;
}

//
// ;rect3
//

struct rect3 {
 union{
  struct {v1 x0,y0,z0;};
  v3 min;
 };
 union{
  struct {v1 x1,y1,z1;};
  v3 max;
 };
};

function rect3
rect3_center_radius(v3 center, v3 radius) {
 radius = absolute(radius);
 return rect3{
  .min=center-radius,
  .max=center+radius,
 };
}

inline b32
contains(rect3 rect, v3 p)
{
 b32 result = ((p.x >= rect.min.x)
               && (p.y >= rect.min.y)
               && (p.z >= rect.min.z)
               && (p.x < rect.max.x)
               && (p.y < rect.max.y)
               && (p.z < rect.max.z));
 return result;
}

inline v3
get_radius(rect3 rect) {
 v3 result = 0.5f * (rect.max - rect.min);
 return result;
}

inline b32
overlap(rect3 a, rect3 b)
{
 b32 separate = ((a.max.x <= b.min.x) || (a.min.x >= b.max.x)
                 || (a.max.y <= b.min.y) || (a.min.y >= b.max.y)
                 || (a.max.z <= b.min.z) || (a.min.z >= b.max.z));
 return !separate;
}

inline v3
getBarycentricCoordinate(rect3 rect, v3 pos)
{
 v3 result;
 v3 dim = rect.max - rect.min;
 result.x = ((pos.x - rect.min.x) / dim.x);
 result.y = ((pos.y - rect.min.y) / dim.y);
 result.z = ((pos.z - rect.min.z) / dim.z);
 return result;
}

typedef i32 i1;

// ;i2 ;i3


myinline v2
V2(i2 v)
{
 return {(f32)v.x, (f32)v.y};
}

myinline i3
operator-(i3 v)
{
 v.x = -v.x;
 v.y = -v.y;
 v.z = -v.z;
 return v;
}

union i4{
 struct{ i32 x,y,z,w; };
 struct{ i32 r,g,b,a; };
 i32 e[4];
 
 i32 &operator[](i32);
};
myinline i32&
i4::operator[](i32 index)
{
 return e[index];
}
union mat3
{
 v3 rows[3];
 v1 e[3][3];
};

union mat4
{
 v4 rows[4];
 v1 e[4][4];
 v1* operator[](i32 i);
};
struct mat4i
{
 union { mat4 forward; mat4 m; };
 union { mat4 inverse; mat4 inv; };
 myinline operator mat4&() { return forward; }  // @ClangSafe
};

myinline v1
get_xscale(mat4 const&mat)
{
 return lengthof(mat.rows[0].xyz);
}

global mat3 mat3_identity = {{
  1,0,0,
  0,1,0,
  0,0,1,
 }};

global mat4 mat4_identity = {{
  1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
 }};

myinline b32 
almost_equal(mat4 const&a, mat4 const&b)
{
 for_i32(i,0,4)
 {
  for_i32(j,0,4)
  {
   if ( !almost_equal(a.e[i][j], b.e[i][j]) ) { return false; }
  }
 }
 return true;
}

myinline v1 *
mat4::operator[](i32 i)
{
 return e[i];
}

function f32
abs_f32(f32 x)
{
 if (x < 0){
  x = -x;
 }
 return(x);
}

function f32
mod_f32(f32 x, i32 m)
{
 f32 whole;
 f32 frac = modff(x, &whole);
 f32 r = f32((i32)(whole) % m) + frac;
 return(r);
}

//~ NOTE(kv): Trig functions
myinline v1 cosine(v1 v01) { return cosf(TAU32 * v01); }
myinline v1 sine(v1 v01){ return sinf(TAU32 * v01); }
myinline v1 arctan2(v1 y, v1 x) { return atan2f(y,x) / TAU32; }
myinline v1 arcsin(v1 v01) { return asinf(v01) / TAU32; }
myinline v1 arccos(v1 v01){ return acosf(v01) / TAU32; }

////////////////////////////////

myinline i2 I2(i32 x, i32 y) { return {x, y}; }
myinline i3 I3(i32 x, i32 y, i32 z) { return {x, y, z}; }
myinline i4 I4(i32 x, i32 y, i32 z, i32 w) { return {x, y, z, w}; }
myinline i4 I4() { return {}; }
myinline i4 I4(i32 x) { return i4{repeat4(x)}; }

myinline v2
V2(v1 x, v1 y)
{
 v2 v = {x, y};
 return(v);
}

myinline v2
cast_V2(i32 x, i32 y)
{
 v2 v = {(v1)x, (v1)y};
 return(v);
}
//

myinline i2
I2(i2 o)
{
 return(I2((i32)o.x, (i32)o.y));
}
myinline v3
V3(i3 o)
{
 return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}

myinline Vec2_i32
operator+(Vec2_i32 a, Vec2_i32 b){
 a.x += b.x;
 a.y += b.y;
 return(a);
}
myinline i2&
operator+=(i2 &a, i2 b){
 a.x += b.x;
 a.y += b.y;
 return(a);
}
function i3
operator+(i3 a, i3 b){
 a.x += b.x;
 a.y += b.y;
 a.z += b.z;
 return(a);
}
function Vec3_i32&
operator+=(Vec3_i32 &a, Vec3_i32 b){
 a.x += b.x;
 a.y += b.y;
 a.z += b.z;
 return(a);
}
function Vec2_i32
operator-(Vec2_i32 a, Vec2_i32 b){
 a.x -= b.x;
 a.y -= b.y;
 return(a);
}
function Vec3_i32
operator-(Vec3_i32 a, Vec3_i32 b){
 a.x -= b.x;
 a.y -= b.y;
 a.z -= b.z;
 return(a);
}

function Vec2_i32&
operator-=(Vec2_i32 &a, Vec2_i32 b){
 a.x -= b.x;
 a.y -= b.y;
 return(a);
}
function Vec3_i32&
operator-=(Vec3_i32 &a, Vec3_i32 b){
 a.x -= b.x;
 a.y -= b.y;
 a.z -= b.z;
 return(a);
}

function Vec2_i32
operator*(i32 s, Vec2_i32 v){
 v.x *= s;
 v.y *= s;
 return(v);
}
function Vec2_i32
operator*(Vec2_i32 v, i32 s){
 v.x *= s;
 v.y *= s;
 return(v);
}
function Vec3_i32
operator*(i32 s, Vec3_i32 v){
 v.x *= s;
 v.y *= s;
 v.z *= s;
 return(v);
}
function Vec3_i32
operator*(Vec3_i32 v, i32 s){
 v.x *= s;
 v.y *= s;
 v.z *= s;
 return(v);
}

function Vec2_i32&
operator*=(Vec2_i32 &v, i32 s){
 v.x *= s;
 v.y *= s;
 return(v);
}
function Vec3_i32&
operator*=(Vec3_i32 &v, i32 s){
 v.x *= s;
 v.y *= s;
 v.z *= s;
 return(v);
}

function Vec2_i32
operator/(Vec2_i32 v, i32 s){
 v.x /= s;
 v.y /= s;
 return(v);
}
function Vec3_i32
operator/(Vec3_i32 v, i32 s){
 v.x /= s;
 v.y /= s;
 v.z /= s;
 return(v);
}
function Vec4_f32
operator/(Vec4_f32 v, f32 s){
 v.x /= s;
 v.y /= s;
 v.z /= s;
 v.w /= s;
 return(v);
}

function Vec2_i32&
operator/=(Vec2_i32 &v, i32 s){
 v.x /= s;
 v.y /= s;
 return(v);
}
function Vec3_i32&
operator/=(Vec3_i32 &v, i32 s){
 v.x /= s;
 v.y /= s;
 v.z /= s;
 return(v);
}
function v2&
operator/=(v2 &v, f32 s){
 v.x /= s;
 v.y /= s;
 return(v);
}
function v3&
operator/=(v3 &v, f32 s){
 v.x /= s;
 v.y /= s;
 v.z /= s;
 return(v);
}
function Vec4_f32&
operator/=(Vec4_f32 &v, f32 s){
 v.x /= s;
 v.y /= s;
 v.z /= s;
 v.w /= s;
 return(v);
}

function bool
operator==(Vec2_i32 a, Vec2_i32 b){
 return(a.x == b.x && a.y == b.y);
}
function bool
operator==(Vec3_i32 a, Vec3_i32 b){
 return(a.x == b.x && a.y == b.y && a.z == b.z);
}
function bool
operator==(Vec4_f32 a, Vec4_f32 b){
 return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

////////////////////////////////

function b32
near_zero(f32 p, f32 epsilon){
 return(-epsilon <= p && p <= epsilon);
}
function b32
near_zero(v2 p, f32 epsilon){
 return(-epsilon <= p.x && p.x <= epsilon &&
        -epsilon <= p.y && p.y <= epsilon);
}
function b32
near_zero(Vec3_f32 p, f32 epsilon){
 return(-epsilon <= p.x && p.x <= epsilon &&
        -epsilon <= p.y && p.y <= epsilon &&
        -epsilon <= p.z && p.z <= epsilon);
}
function b32
near_zero(v4 p, v1 epsilon)
{
 return(-epsilon <= p.x && p.x <= epsilon &&
        -epsilon <= p.y && p.y <= epsilon &&
        -epsilon <= p.z && p.z <= epsilon &&
        -epsilon <= p.w && p.w <= epsilon);
}

function b32
near_zero(f32 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(v2 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(Vec3_f32 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(Vec4_f32 p){ return(near_zero(p, epsilon_f32)); }

////////////////////////////////

function v2
lerp(v2 a, f32 t, v2 b){
 return(a + (b-a)*t);
}

myinline v3
lerp(v3 a, v1 t, v3 b){
 return(a + (b-a)*t);
}

function v1
unlerp(v1 a, v1 x, v1 b) {
 v1 r = 0.f;
 if (b != a) {
  r = (x - a)/(b - a);
 }
 return(r);
}

myinline v1
clamp01(v1 v)
{
 macro_clamp01(v);
 return v;
}

myinline v1
unlerp01(v1 a, v1 v, v1 b)
{
 return clamp01( unlerp(a,v,b) );
}

function v1
smoothstep(v1 a, v1 x, v1 b)
{
 if (a != b)
 {
  v1 t = clamp01((x - a) / (b - a));
  return t*t*(3.f - (2.f*t));
 }
 else if (x > a) { return 1.f; }
 else { return 0.f; }
}

////////////////////////////////

function bool
operator==(Rect_i32 a, Rect_i32 b){
 return(a.p0 == b.p0 && a.p1 == b.p1);
}
function bool
operator==(Rect_f32 a, Rect_f32 b){
 return(a.p0 == b.p0 && a.p1 == b.p1);
}

function v2
rect_center(Rect_f32 r){
 return((r.p0 + r.p1)*0.5f);
}
myinline v2 V2(v1 value) { return v2{repeat2(value)}; }
myinline v3 V3(v1 value) { return v3{repeat3(value)}; }
myinline v4 V4(v1 value) { return v4{repeat4(value)}; }
myinline v2 V2() { return v2{}; }
myinline v3 V3() { return v3{}; }
myinline v4 V4() { return v4{}; }

inline v1
dot(v4 const v, v4 const u)
{
 v1 result = v.x*u.x + v.y*u.y + v.z*u.z + v.w*u.w;
 return result;
}

function v3 
matvmul3(mat3 const&matrix, v3 v)
{
 v1 row0 = dot(v, matrix.rows[0]);
 v1 row1 = dot(v, matrix.rows[1]);
 v1 row2 = dot(v, matrix.rows[2]);
 v3 result = V3(row0, row1, row2);
 return result;
}

function mat3
operator*(mat3 const&A, mat3 const&B)
{
 mat3 R = {};
 for_i32(r,0,3) // NOTE(casey): Rows (of A)
 {
  for_i32(c,0,3) // NOTE(casey): Column (of B)
  {
   for_i32(i,0,3) // NOTE(casey): i = Column of A = Row of B
   {
    R.e[r][c] += A.e[r][i] * B.e[i][c];
   }
  }
 }
 return(R);
}

// NOTE: Everyone should get fired, for writing compilers that do stupid things.
function v4
operator*(mat4 const&matrix, v4 v)
{
 v4 result = {};
 for_i32(r,0,4)
 {
  for_i32(i,0,4)
  {
   result.v[r] += matrix.e[r][i] * v.v[i];
  }
 }
 return result;
}

function mat4
matmul(mat4 const*A, mat4 const*B) {
 mat4 R = {};
 for_i32(r,0,4) // NOTE(casey): Rows (of A)
 {
  for_i32(c,0,4) // NOTE(casey): Column (of B)
  {
   for_i32(i,0,4) // NOTE(casey): i = Column of A = Row of B
   {
    R.e[r][c] += A->e[r][i] * B->e[i][c];
   }
  }
 }
 return(R);
}
myinline mat4
matmul(mat4 const&A, mat4 const&B)
{
 return matmul(&A,&B);
}
//NOTE: This actually allows us to "pass by value"
// And clang actually does the right optimization in debug, which is refreshing.
myinline mat4
operator*(mat4 const&A, mat4 const&B)
{
 return matmul(&A,&B);
}

inline mat3
to_mat3(mat4 const&m)
{
 mat3 result;
 for_i32(index,0,3)
 {
  result.rows[index] = m.rows[index].xyz;
 }
 return result;
}

myinline v3
mat4vert_div(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v,1.f);
 return Av.xyz / Av.w;
}
myinline v3
mat4vert(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v, 1.f);
 return Av.xyz;
}
// IMPORTANT IMPORTANT IMPORTANT: I am a bad person! But there's no way around it!
myinline v3
operator*(mat4 const&A, v3 v)
{
 return mat4vert(A,v);
}

myinline v3
mat4vec(mat4 const&A, v3 v)
{
 v4 result = A * V4(v,0.f);
 return result.xyz;
}

global mat4i mat4i_identity = {mat4_identity, mat4_identity};

function mat4i
invert(mat4i const&in)
{
 return mat4i{in.inverse, in.forward};
}

function mat4
mat4_scales(v1 sx, v1 sy, v1 sz)
{
 mat4 result = {{
   sx,0,0,0,
   0,sy,0,0,
   0,0,sz,0,
   0,0,0,1,
  }};
 return result;
}

myinline mat4
mat4_scales(v3 scales)
{
 return mat4_scales(v3_expand(scales));
}

myinline mat4
mat4_scale(v1 s)
{
 return mat4_scales(V3(s));
}

myinline mat4i
mat4i_scales(v3 s)
{
 mat4i result;
 result.forward = mat4_scales(s);
 result.inverse = mat4_scales(1.f/s.x, 1.f/s.y, 1.f/s.z);
 return result;
}

myinline mat4i
mat4i_scales(v1 sx, v1 sy, v1 sz)
{
 return mat4i_scales(V3(sx,sy,sz));
}

myinline mat4i
mat4i_scale(v1 s)
{
 mat4i result;
 result.forward = mat4_scale(s);
 result.inverse = mat4_scale(1.f/s);
 return result;
}

function mat4 
transpose(mat4 mat){
 for_i32(r,0,4) { 
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}
function mat3
transpose(mat3 mat){
 for_i32(r,0,3) {
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}
function mat4
mat4_columns(v3 x, v3 y, v3 z, v3 w){
 mat4 inverse;
 inverse.rows[0] = V4(x,0);
 inverse.rows[1] = V4(y,0);
 inverse.rows[2] = V4(z,0);
 inverse.rows[3] = V4(w,1);
 
 return transpose(inverse);
}
function mat4i
mat4i_columns(v3 x, v3 y, v3 z, v3 w){
 mat4i result;
 mat4 &inverse = result.inverse;
 inverse.rows[0] = V4(x,0);
 inverse.rows[1] = V4(y,0);
 inverse.rows[2] = V4(z,0);
 inverse.rows[3] = V4(w,1);
 result.forward = transpose(inverse);
 return result;
}

function v4
get_column(mat4 const&m, i32 index)
{
 v4 result;
 for_i32(i,0,4)
 {
  result[i] = m.e[i][index];
 }
 return result;
}

myinline v3
get_translation(mat4 const&mat)
{
 return get_column(mat, 3).xyz;
}

myinline mat3
mat3_scale(v1 s)
{
 mat3 result = {{
   s,0,0,
   0,s,0,
   0,0,s,
  }};
 return result;
}

myinline v3
operator*(mat3 const&m, v3 v)
{
 return matvmul3(m,v);
}

function mat4
to_mat4(mat3 mat, v3 translation=V3())
{
 mat4 result;
 for_i32(index,0,3)
 {
  result.rows[index] = V4(mat.rows[index], translation[index]);
 }
 result.rows[3] = V4(0,0,0,1);
 return result;
}

// NOTE(kv): I'm not sure what this is for.
struct TRS
{
 v3   translation;
 mat3 rotation;
 v1   scale;
};

function mat3
operator*(v1 s, mat3 mat)
{
 for_i32(r,0,3)
 {
  for_i32(c,0,3)
  {
   mat.e[r][c] *= s;
  }
 }
 return mat;
}

function mat3
get_rotation(mat4 const&transform)
{
 v1 scale = get_xscale(transform);
 return (1.f/scale)*to_mat3(transform);
}

function TRS
trs_decompose(mat4 const&transform)
{
 v1 scale = get_xscale(transform);
 TRS out;
 out.translation = get_translation(transform);
 out.rotation    = (1.f/scale) * to_mat3(transform);
 out.scale       = scale;
 return out;
}

function mat4
mat4_translate(v3 vector)
{
 mat4 result = {{
   1,0,0,vector.x,
   0,1,0,vector.y,
   0,0,1,vector.z,
   0,0,0,1,
  }};
 return result;
}

myinline mat4i
mat4i_translate(v3 vector)
{
 mat4i result;
 result.forward = mat4_translate(vector);
 result.inverse = mat4_translate(-vector);
 return result;
}

function void
rotation_pivot_helper(mat4 *matrix, v3 pivot)
{
 if ( pivot != v3{} )
 {
  v3 translation = mat4vec(*matrix, (-pivot)) + pivot;
  for_i32 (index,0,3) { matrix->e[index][3] = translation[index]; }
 }
}
//
inline void
rotation_pivot_helper(mat4i *matrix, v3 pivot)
{
 rotation_pivot_helper(&matrix->forward, pivot);
 rotation_pivot_helper(&matrix->inverse, pivot);
}

function mat4
rotateX(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   1, 0, 0, 0, 
   0, c,-s, 0,
   0, s, c, 0,
   0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateX(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateX(turn, pivot);
 result.inverse = rotateX(-turn, pivot);
 return result;
}

function mat4
rotateY(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   +c, 0, s, 0, 
   +0, 1, 0, 0,
   -s, 0, c, 0,
   +0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateY(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateY(turn, pivot);
 result.inverse = rotateY(-turn, pivot);
 return result;
}

function mat4
rotateZ(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   c, -s, 0, 0, 
   s, c, 0, 0,
   0, 0, 1, 0,
   0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateZ(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateZ(turn, pivot);
 result.inverse = rotateZ(-turn, pivot);
 return result;
}

//////////////////////////////////////////////////

myinline mat4i
mat4i_rotate(mat3 rot)
{
 mat4i result;
 result.forward = to_mat4(rot);
 result.inverse = to_mat4(transpose(rot));
 return result;
}

function mat4i
matmul(mat4i const*A, mat4i const*B)
{
 mat4i result;
 result.forward = matmul(&A->forward, &B->forward);
 result.inverse = matmul(&B->inverse, &A->inverse);
 return result;
}
myinline mat4i
matmul(mat4i const& A, mat4i const& B)
{
 return matmul(&A, &B);
}
//NOTE: Compose transformations
myinline mat4i
operator*(mat4i const& A, mat4i const& B)
{
 return matmul(&A, &B);
}

myinline v2 arm2(v1 turn)
{
 return v2{cosine(turn), sine(turn)};
}

myinline v3 V3x(v1 x) { v3 result = {}; result.x=x; return result; }
myinline v3 V3y(v1 y) { v3 result = {}; result.y=y; return result;  }
myinline v3 V3z(v1 z) { v3 result = {}; result.z=z; return result; }
myinline v3 setx(v3 v, v1 x) { v.x=x; return v; }
myinline v3 sety(v3 v, v1 y) { v.y=y; return v; }
myinline v3 setz(v3 v, v1 z) { v.z=z; return v; }
myinline v3 addx(v3 v, v1 x) { v.x+=x; return v; }
myinline v3 addy(v3 v, v1 y) { v.y+=y; return v; }
myinline v3 addz(v3 v, v1 z) { v.z+=z; return v; }
myinline v3 zeroX(v3 value) { value.x=0; return value; };

myinline v1 i2f6 (i32 integer) { return v1(integer) / 6.f; }
myinline v1 i2f(i32 integer, v1 div) { return v1(integer) / div; }
myinline v4
i2f6(i4 vi)
{
 v4 result;
 for_i32(index,0,4) { result[index] = v1(vi[index]) / 6.f; }
 return result;
}
myinline v1
step(v1 edge, v1 x)
{
 return (x < edge) ? 0.f : 1.f;
}


myinline v3
step(v3 edge, v3 v)
{
 return V3(step(edge.x, v.x),
           step(edge.y, v.y),
           step(edge.z, v.z));
}

myinline i1
signof(i1 x)
{
 return (x == 0 ? 0 :
         x > 0  ? 1 :
         -1);
}

myinline v1
signof(v1 x)
{
 return (x == 0.f ? 0.f :
         x > 0.f  ? 1.f :
         -1.f);
}
myinline v3
signof(v3 v)
{
 return V3(signof(v.x),
           signof(v.y),
           signof(v.z));
}


function mat4i
mat4i_rotate_tpr(v1 theta, v1 phi, v1 roll, v3 pivot={})
{// NOTE: Roll around z, then rotate around x, then rotate around y
 // NOTE Weird, in the inverse, we want to the roll_inv *last*
 // and so we endup doing the roll *first* in the forward direction.
 
 phi  *= -1.f;  // NOTE: But the rotation axes are "mirrored" since we want camera control to be intuitive
 roll *= -1.f;
 
 mat4i result;
 {
  v1 ct = cosine(theta);
  v1 st = sine  (theta);
  v1 cp = cosine(phi);
  v1 sp = sine  (phi);
  //NOTE: we're just doing a matmul by ourselves here, for reasons.
  result.inverse = {{
    ct,     0,   -st,    0,
    sp*st,  cp,   ct*sp, 0,
    cp*st, -sp,   cp*ct, 0,
    0,      0,    0,     1,}};
 }
 
 result.inverse = rotateZ(-roll)*result.inverse;
 result.forward = transpose(result.inverse);
 rotation_pivot_helper(&result, pivot);
 
 return result;
}

inline v3
tpr_point(v1 theta, v1 phi)
{
 return mat4i_rotate_tpr(theta,phi,0,V3()) * V3(0,0,1);
}

global_const mat4 mat4_negateX = {{
  -1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
 }};

myinline v3 
negateX(v3 vert){
 return V3(-vert.x, vert.y, vert.z);
}
//NOTE(kv) I think this is like multiple by a negateX matrix on the right.
inline mat4
negateX(mat4 mat){
 for_i32(row,0,4) { mat.e[row][0] *= -1.f; }
 return mat;
}
inline mat4i
negateX(mat4i mat){
 for_i32(row,0,4){ mat.forward.e[row][0] *= -1.f; }
 for_i32(col,0,4){ mat.inverse.e[0][col] *= -1.f; }
 return mat;
}
function mat4
remove_translation(mat4 result){
 result[0][3] = 0.f;
 result[1][3] = 0.f;
 result[2][3] = 0.f;
 return result;
}
//-
struct Loaded_Bitmap 
{
 u8 *data;
 i2  dim;
 i32 pitch;
};
function v4
linearToSrgb(v4 linear)
{
 v4 result;
 result.r = square_root(linear.r);
 result.g = square_root(linear.g);
 result.b = square_root(linear.b);
 result.a = linear.a;
 return result;
}
function u32
pack_sRGBA(v4 color)
{
 // linear to srgb
 color.r = square_root(color.r);
 color.g = square_root(color.g);
 color.b = square_root(color.b);
 u32 result = ((u32)(color.a*255.0f + 0.5f) << 24
               | (u32)(color.b*255.0f + 0.5f) << 16
               | (u32)(color.g*255.0f + 0.5f) << 8
               | (u32)(color.r*255.0f + 0.5f));
 return result;
}
function v4
argb_unpack(ARGB_Color color)
{
 v4 result;
 result.a = ((color >> 24) & 0xFF)/255.f;
 result.r = ((color >> 16) & 0xFF)/255.f;
 result.g = ((color >> 8)  & 0xFF)/255.f;
 result.b = ((color >> 0)  & 0xFF)/255.f;
 return(result);
}

function ARGB_Color
argb_pack(v4 color)
{
 ARGB_Color result =
 ((u8)(color.a*255) << 24) |
 ((u8)(color.r*255) << 16) |
 ((u8)(color.g*255) << 8) |
 ((u8)(color.b*255) << 0);
 return(result);
}

function ARGB_Color
color_blend(ARGB_Color a, f32 t, ARGB_Color b)
{
 Vec4_f32 av = argb_unpack(a);
 Vec4_f32 bv = argb_unpack(b);
 Vec4_f32 v = lerp(av, t, bv);
 return(argb_pack(v));
}

myinline v3 
clamp01(v3 v)
{
 for_i32(i,0,3)
 {
  macro_clamp01(v.v[i]);
 }
 return v;
}
function Vec2_i32
rect2i_dim(Rect_i32 r)
{
 Vec2_i32 v = {r.x1 - r.x0, r.y1 - r.y0};
 return(v);
}
function Vec2_i32
rect2_half_dim(Rect_i32 r){
 return(rect2i_dim(r)/2);
}
function v2
rect2_half_dim(Rect_f32 r){
 return(get_dim(r)*0.5f);
}
function Vec2_i32
rect_center(Rect_i32 r){
 return((r.p0 + r.p1)/2);
}


//-