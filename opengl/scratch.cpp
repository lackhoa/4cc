//-
package main
import "fmt"

func foo(input int) int {
 return input + 5;
}
func main() {
 var x int = 5;
 var y *int = &x;
	fmt.Println("Hello, sailor %d", foo(y));
}
//-