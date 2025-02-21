//
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

function void
do_image_test()
{
 char *filename = "G:\\My Drive\\Videos\\Slides\\Whats in here.jpg";
 i32 image_width;
 i32 image_height;
 i32 ncomp = 4;
 u8 *data = 0;
 data = stbi_load(filename, &image_width, &image_height, 0, ncomp);
 
 {// TODO Put it in the desktop
  char *out_filename = "G:\\My Drive\\Videos\\Slides\\test output.png";
  i32 stride = ncomp * image_width;
  stbi_write_png(out_filename, image_width, image_height, ncomp, data, stride);
 }
 
 free(data);
}
//