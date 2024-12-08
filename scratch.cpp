In my current effort to migrate my code from C++ to C,
I'm encountering problems from the use of references.

Say my matrix multiplication code looks like this:

```
struct mat4
{
 float e[4][4];
};

function mat4
matmul(mat4 const&A, mat4 const&B)
{
 mat4 R = {};
 for(int r=0; r < 4; r++) {
  for(int c=0; c < 4; c++) {
   for(int i=0; i < 4; i++) {
    R.e[r][c] += A.e[r][i] * B.e[i][c];
   }
  }
 }
 return(R);
}
```

A straightforward transformation would be like this

```
function mat4
matmul(mat4 *A, mat4 *B)
{
 mat4 R = {};
 for(int r...) {
  for(int c...) {
   for(int i...) {
    R.e[r][c] += A->e[r][i] * B->e[i][c];
   }
  }
 }
 return(R);
}
```

Technically it does the same thing, but now math code has become impossible to write.
In particular, you can't write this anymore.

```
mat4 *A;
mat4 *B;
mat4 *C;
mat4 D = matmul(matmul(A,B), C);
```

I see in Handmade Hero, Casey passes the argument matrices in by value.
This may be fine in optimized code but might be slow in debug mode.

So maybe this is bad?
//
