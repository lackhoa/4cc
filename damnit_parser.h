// ;type_sizes  (TODO(kv) THIS made the parser break, and I don't know why!)
global_const i32 type_sizes[Type_Count] = {
 0,
#define X(T)  sizeof(T),
 X_Basic_Types(X)
#undef X
};

global_const String type_strings[Type_Count] = {
 String{},
#define X(T)  strlit(#T),
 X_Basic_Types(X)
#undef X
};

//~eof