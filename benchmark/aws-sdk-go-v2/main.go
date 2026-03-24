package main

/*
#include <stdlib.h>
*/
import "C"
import (
	"context"
	"fmt"
	"io"
	"log"
	"os"
	"runtime"
	"runtime/cgo"
	"unsafe"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

var p runtime.Pinner // yolo

func initialize_client(access, secret, endpoint string) *s3.Client {
	cfg, err := config.LoadDefaultConfig(context.TODO(),
		config.WithCredentialsProvider(credentials.NewStaticCredentialsProvider(access, secret, "")),
		config.WithBaseEndpoint("http://127.0.0.1:9000/"),
		config.WithDefaultRegion("eu-west-2"),
	)
	if err != nil {
		log.Fatal(err)
	}
	client := s3.NewFromConfig(cfg, func(opt *s3.Options) {
		opt.UsePathStyle = true
		opt.DisableLogOutputChecksumValidationSkipped = true
	})
	return client
}

func do_get_object(client *s3.Client, key string) string {
	result, err := client.GetObject(context.TODO(), &s3.GetObjectInput{
		Bucket: aws.String("bucket"),
		Key:    aws.String(key),
	})
	if err != nil {
		fmt.Printf("fatal: go get_object %v\n", err.Error())
		os.Exit(1)
	}
	defer result.Body.Close()
	content, err := io.ReadAll(result.Body)
	if err != nil {
		fmt.Printf("fatal: go get_object %v\n", err.Error())
		os.Exit(1)
	}
	return string(content[:])
}

//export initClient
func initClient(access *C.char, secret *C.char, endpoint *C.char) unsafe.Pointer {
	accessStr := C.GoString(access)
	secretStr := C.GoString(secret)
	endpointStr := C.GoString(endpoint)

	var sdkClient *s3.Client = initialize_client(accessStr, secretStr, endpointStr)

	handler := cgo.NewHandle(sdkClient)
	h_ptr := unsafe.Pointer(&handler)
	p.Pin(h_ptr)

	return h_ptr
}

//export get_object
func get_object(handle unsafe.Pointer, key *C.char) *C.char {
	defer p.Unpin()
	h := *(*cgo.Handle)(handle)
	keyStr := C.GoString(key)
	contents := do_get_object(h.Value().(*s3.Client), keyStr)
	return C.CString(contents)
}

func main() {}
