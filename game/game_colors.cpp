#pragma once

function v3 
hue2rgb(v1 hue)
{// NOTE(kv) Source: https://www.ronja-tutorials.com/post/041-hsv-colorspace/
 hue = 6 * cycle01(hue);
 v1 r = absolute(hue - 3) - 1;
 v1 g = 2 - absolute(hue - 2);
 v1 b = 2 - absolute(hue - 4);
 macro_clamp01(r);
 macro_clamp01(g);
 macro_clamp01(b);
 return V3(r,g,b);
}

function v3 
hsv_to_srgb(v1 h, v1 s, v1 v)
{// TODO(kv) Why is it "srgb" specifically?
 // hue
 v3 rgb = hue2rgb(h);
 for_i32(index,0,3)
 {// saturation
  rgb[index] = lerp(1.f, s, rgb[index]);
 }
 // value
 rgb *= v;
 return rgb;
}

myinline v3 
hsv_to_srgb(v3 hsv)
{
 return hsv_to_srgb(v3_expand(hsv));
}

inline v4
srgb_to_linear(v4 input)
{
 input.r = srgb_to_linear1(input.r);
 input.g = srgb_to_linear1(input.g);
 input.b = srgb_to_linear1(input.b);
 return input;
}
myinline argb
srgb_to_linear(argb input)
{// TODO(kv) This doesn't make any sense at all!
 // if you're using packed colors, you have to use srgb!
 v4 value = argb_unpack(input);
 return argb_pack( srgb_to_linear(value) );
}
myinline v4
srgb_to_linear(v3 input)
{
 return srgb_to_linear( V4(input,1.0f) );
}
myinline v4
srgb_to_linear(v1 r, v1 g, v1 b)
{
 return srgb_to_linear( V4(r,g,b,1.0f) );
}

function argb
hsv_to_argb(v1 h, v1 s, v1 v)
{
 return argb_pack(srgb_to_linear(hsv_to_srgb(h,s,v)));
}

inline argb 
linear_to_srgb(argb input)
{
 v4 value = argb_unpack(input);
 value.r = linear_to_srgb1(value.r);
 value.g = linear_to_srgb1(value.g);
 value.b = linear_to_srgb1(value.b);
 return argb_pack(value);
}

function v4
argb_lightness(v4 color, v1 lightness)
{
 color.rgb *= lightness;
 for_i32(i, 0, 3)
 {
  ClampBot(color.e[i], 0.f);
  ClampTop(color.e[i], 1.f);
 }
 return color;
}
function argb
argb_lightness(argb linear_color, v1 lightness)
{// NOTE Boy I really hate these...
 v4 color_v4 = argb_unpack(linear_color);
 color_v4 = argb_lightness(color_v4, lightness);
 return argb_pack(color_v4);
}
function v4
mk_v4_rgb(v1 r, v1 g, v1 b)
{
 v4 result;
 result.r = r;
 result.g = g;
 result.b = b;
 result.a = 1.f;
 return result;
}
// NOTE: We now have the honor of converting colors to linear
// TODO(kv) If we're gonna use linear color, just use v4!
global argb linear_argb_yellow      = srgb_to_linear(0xFF777700);
global argb sargb_bright_yellow = 0xFF998963;
global argb linear_argb_dim_red     = srgb_to_linear(0xFF886666);
global argb linear_argb_red         = argb_pack(srgb_to_linear(hsv_to_srgb(V3(0.f, 0.5739f, 0.4987f))));
global argb linear_argb_green       = argb_pack({0,.5,0,1});
global argb linear_argb_dark_green  = srgb_to_linear(0xff006400);
global argb linear_argb_blue        = srgb_to_linear(0xFF586890);
global argb sargb_bright_blue = 0xFF798999;
global argb argb_black       = 0xff000000;
global v4   v4_white         = {1,1,1,1};
global argb argb_white       = 0xffffffff;
global argb argb_marble = 0xFFacaeb5;
global argb linear_argb_marble    = srgb_to_linear(argb_marble);
global argb linear_argb_silver    = argb_lightness(argb_marble, 0.8723f);
global argb linear_argb_dark_blue = srgb_to_linear(0xFF282c38);

//~ eof