#include <stdio.h>
#include <assert.h>

int main () {
 char *file1 = __builtin_FILE();
 char *file2 = __builtin_FILE();
 assert(file1 == file2);
 return 0;
}
