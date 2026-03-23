package main

/*
#include <stdlib.h>
*/
import "C"
import "unsafe"

//export get_object
func get_object(key *C.char) *C.char {
	k := C.GoString(key)
	s := "go: Key: " + k
	return C.CString(s)
}

//export free_object
func free_object(ptr *C.char) {
	C.free(unsafe.Pointer(ptr))
}

func main() {}
