// NOTE: If polygon count is too low, triangles occluding bezier curves will create UGLY patterns
global i32 PATCH_NSLICE;
global i32 BEZIER_POLY_NSLICE;
global i32 CIRCLE_NSLICE;
global v1 DEFAULT_NSLICE_PER_METER;

internal void 
update_game_config()
{
 PATCH_NSLICE             = fval(16);
 BEZIER_POLY_NSLICE       = fval(16);
 CIRCLE_NSLICE            = fval(8);
 DEFAULT_NSLICE_PER_METER = fval(2.2988f) * 100.f;
}