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
//-