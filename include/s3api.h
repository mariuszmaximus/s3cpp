#ifndef S3API_H
#define S3API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* code;
    char* message;
    char* resource;
    int request_id;
} s3_error;

typedef struct {
    const char* bucket_region;
    const char* continuation_token;
    int* max_buckets;
    const char* prefix;
} s3_list_buckets_options;

typedef struct {
    char* bucket_arn;
    char* bucket_region;
    char* creation_date;
    char* name;
} s3_bucket;

typedef struct {
    char* display_name;
    char* id;
} s3_owner;

typedef struct {
    s3_bucket* buckets;
    size_t bucket_count;
    s3_owner owner;
    char* continuation_token;
    char* prefix;
} s3_list_buckets_result;

typedef struct {
    bool is_error;
    union {
        s3_list_buckets_result result;
        s3_error error;
    } data;
} s3_list_buckets_response;

typedef enum {
    S3_ADDRESSING_VIRTUAL_HOSTED = 0,
    S3_ADDRESSING_PATH_STYLE = 1
} s3_addressing_style;

typedef struct s3_client s3_client;

s3_client* s3_client_create(const char* access_key, const char* secret_key);
s3_client* s3_client_create_with_region(const char* access_key, const char* secret_key, const char* region);
s3_client* s3_client_create_with_endpoint(const char* access_key, const char* secret_key, const char* endpoint, s3_addressing_style style);
void s3_client_destroy(s3_client* client);

s3_list_buckets_response list_buckets(s3_client* client, const s3_list_buckets_options* options);

void s3_free_string(char* str);
void s3_free_error(s3_error* error);
void s3_free_list_buckets_result(s3_list_buckets_result* result);
void s3_free_list_buckets_response(s3_list_buckets_response* response);

#ifdef __cplusplus
}
#endif

#endif /* S3API_H */
