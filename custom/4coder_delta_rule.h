/*
4coder_delta_rule.h - Types for built in delta rules and delta rule helpers.
*/

// TOP

#if !defined(FCODER_DELTA_RULE_H)
#define FCODER_DELTA_RULE_H

union Delta_Context_Header{
    Buffer_Point point;
    v2 p;
};
struct Buffer_Point_Delta_Result{
    Buffer_Point point;
    b32 still_animating;
};
struct v2_Delta_Result{
    v2 p;
    b32 still_animating;
};

struct Smooth_Step{
    f32 p;
    f32 v;
};

#endif

// BOTTOM

