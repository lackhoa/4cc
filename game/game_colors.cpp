#pragma once

internal v3 
hue2rgb(v1 hue)
{
 hue = cycle01(hue); //only use fractional part of hue, making it loop
 v1 r = absolute(hue * 6 - 3) - 1; //red
 v1 g = 2 - absolute(hue * 6 - 2); //green
 v1 b = 2 - absolute(hue * 6 - 4); //blue
 macro_clamp(0,r,1);
 macro_clamp(0,g,1);
 macro_clamp(0,b,1);
 v3 rgb = V3(r,g,b); //combine components
 return rgb;
}

internal v3 
hsv_to_srgb(v1 h, v1 s, v1 v)
{
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

force_inline v3 
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
force_inline argb
srgb_to_linear(argb input)
{
 v4 value = argb_unpack(input);
 return argb_pack( srgb_to_linear(value) );
}
force_inline v4
srgb_to_linear(v3 input)
{
 return srgb_to_linear( V4(input,1.0f) );
}
force_inline v4
srgb_to_linear(v1 r, v1 g, v1 b)
{
 return srgb_to_linear( V4(r,g,b,1.0f) );
}

internal argb
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

internal argb
argb_lightness(argb color, v1 lightness)
{
 v4 result = argb_unpack(color);
 result.rgb *= lightness;
 return argb_pack(result);
}
// NOTE: We now have the honor of converting colors to linear
global argb argb_yellow      = srgb_to_linear(0xFF777700);
global argb argb_dim_red     = srgb_to_linear(0xFF886666);
global argb argb_red         = argb_pack(srgb_to_linear(hsv_to_srgb(V3(0.f, 0.5739f, 0.4987f))));
global argb argb_green       = argb_pack({0,.5,0,1});
global argb argb_dark_green  = srgb_to_linear(0xff006400);
global argb argb_blue        = srgb_to_linear(0xFF586890);
global argb argb_black       = 0xff000000;
global v4  v4_white          = {1,1,1,1};
global argb argb_white       = 0xffffffff;
global argb argb_marble_srgb = 0xFFacaeb5;
global argb argb_marble      = srgb_to_linear(argb_marble_srgb);
global argb argb_silver      = argb_lightness(argb_marble, 0.8723f);
global argb argb_dark_blue   = srgb_to_linear(0xFF282c38);

//~ eof