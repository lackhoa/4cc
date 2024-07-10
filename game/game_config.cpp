
#if AD_COMPILING_DRIVER
#    define storage  global_extern
#else
#    define storage  extern
#endif

storage i1 BEZIER_POLY_NSLICE;
storage v1 DEFAULT_NSLICE_PER_METER;
storage b32 debug_frame_time_on;

#undef storage

//~