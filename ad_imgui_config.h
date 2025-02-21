#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_USE_STB_SPRINTF

union v2;
union v4;

#define IM_VEC2_CLASS_EXTRA \
operator v2();

#define IM_VEC4_CLASS_EXTRA \
operator v4();

//~
