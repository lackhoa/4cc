
// You can edit this code!
// Click here and start typing.
package main

import "fmt"

type Foo struct{
 x int
}

type Bar struct{
 y int
  Foo
}

func function_that_works_on_foo(foo *Foo) {
 foo.x = 8
}

func main() {
 var bar Bar;
 bar.x = 5;
 bar.y = 6;
 function_that_works_on_foo(&bar)
	fmt.Println("Hello %d %d", bar.x, bar.y);
}
///