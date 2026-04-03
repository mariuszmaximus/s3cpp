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
	s3types "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"strings"
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

func do_create_bucket(client *s3.Client, bucket string) {
	_, err := client.CreateBucket(context.TODO(), &s3.CreateBucketInput{
		Bucket:                    aws.String(bucket),
		CreateBucketConfiguration: &s3types.CreateBucketConfiguration{},
	})
	if err != nil {
		fmt.Printf("fatal: go create_bucket %v\n", err.Error())
		os.Exit(1)
	}
}

func do_put_object(client *s3.Client, bucket, key, contents string) {
	_, err := client.PutObject(context.TODO(), &s3.PutObjectInput{
		Bucket: aws.String(bucket),
		Key:    aws.String(key),
		Body:   strings.NewReader(contents),
	})
	if err != nil {
		fmt.Printf("fatal: go put_object %v\n", err.Error())
		os.Exit(1)
	}
}

func do_get_object(client *s3.Client, bucket, key string) string {
	result, err := client.GetObject(context.TODO(), &s3.GetObjectInput{
		Bucket: aws.String(bucket),
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

//export init_client
func init_client(access *C.char, secret *C.char, endpoint *C.char) unsafe.Pointer {
	accessStr := C.GoString(access)
	secretStr := C.GoString(secret)
	endpointStr := C.GoString(endpoint)

	var sdkClient *s3.Client = initialize_client(accessStr, secretStr, endpointStr)

	handler := cgo.NewHandle(sdkClient)
	h_ptr := unsafe.Pointer(&handler)
	p.Pin(h_ptr)

	return h_ptr
}

//export create_bucket
func create_bucket(handle unsafe.Pointer, bucketName *C.char) {
	defer p.Unpin()
	h := *(*cgo.Handle)(handle)
	do_create_bucket(h.Value().(*s3.Client), C.GoString(bucketName))
}

//export put_object
func put_object(handle unsafe.Pointer, bucket *C.char, key *C.char, contents *C.char) {
	defer p.Unpin()
	h := *(*cgo.Handle)(handle)
	do_put_object(h.Value().(*s3.Client), C.GoString(bucket), C.GoString(key), C.GoString(contents))
}

//export get_object
func get_object(handle unsafe.Pointer, bucket *C.char, key *C.char) *C.char {
	defer p.Unpin()
	h := *(*cgo.Handle)(handle)
	bucketStr := C.GoString(bucket)
	keyStr := C.GoString(key)
	contents := do_get_object(h.Value().(*s3.Client), bucketStr, keyStr)
	return C.CString(contents)
}

func main() {}
