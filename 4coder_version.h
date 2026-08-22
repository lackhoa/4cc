#define MAJOR 4
#define MINOR 1
#define PATCH 8

// string

#define ST__(s) #s
#define ST_(s) ST__(s)
#define MAJOR_STR ST_(MAJOR)
#define MINOR_STR ST_(MINOR)
#define PATCH_STR ST_(PATCH)

// string

#define WINDOW_NAME "4coder"

#define L_WINDOW_NAME L"4coder: "