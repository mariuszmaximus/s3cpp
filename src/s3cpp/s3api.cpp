#include <cstdlib>
#include <cstring>
#include <s3api.h>
#include <s3cpp/s3.h>

extern "C" {

s3_client* s3_client_create(const char* access_key, const char* secret_key) {
    return reinterpret_cast<s3_client*>(new S3Client(access_key, secret_key));
}

s3_client* s3_client_create_with_region(const char* access_key, const char* secret_key, const char* region) {
    return reinterpret_cast<s3_client*>(new S3Client(access_key, secret_key, region));
}

s3_client* s3_client_create_with_endpoint(const char* access_key, const char* secret_key, const char* endpoint, s3_addressing_style style) {
    auto cpp_style = static_cast<S3AddressingStyle>(style);
    return reinterpret_cast<s3_client*>(new S3Client(access_key, secret_key, endpoint, cpp_style));
}

void s3_client_destroy(s3_client* client) {
    delete reinterpret_cast<S3Client*>(client);
}

s3_list_buckets_response list_buckets(s3_client* client, const s3_list_buckets_options* options) {
    S3Client* s3_client = reinterpret_cast<S3Client*>(client);

    ListBucketsInput input;
    if (options) {
        if (options->bucket_region)
            input.BucketRegion = options->bucket_region;
        if (options->continuation_token)
            input.ContinuationToken = options->continuation_token;
        if (options->max_buckets)
            input.MaxBuckets = *options->max_buckets;
        if (options->prefix)
            input.Prefix = options->prefix;
    }

    std::expected<ListAllMyBucketsResult, Error> result = s3_client->ListBuckets(input);

    s3_list_buckets_response response {};
    if (result.has_value()) {
        response.is_error = false;
        const auto& cpp_result = result.value();

        response.data.result.bucket_count = cpp_result.Buckets.size();
        response.data.result.buckets = static_cast<s3_bucket*>(malloc(sizeof(s3_bucket) * cpp_result.Buckets.size()));

        for (size_t i = 0; i < cpp_result.Buckets.size(); ++i) {
            const auto& cpp_bucket = cpp_result.Buckets[i];
            response.data.result.buckets[i].bucket_arn = strdup(cpp_bucket.BucketARN.c_str());
            response.data.result.buckets[i].bucket_region = strdup(cpp_bucket.BucketRegion.c_str());
            response.data.result.buckets[i].creation_date = strdup(cpp_bucket.CreationDate.c_str());
            response.data.result.buckets[i].name = strdup(cpp_bucket.Name.c_str());
        }

        response.data.result.owner.display_name = strdup(cpp_result.Owner.DisplayName.c_str());
        response.data.result.owner.id = strdup(cpp_result.Owner.ID.c_str());
        response.data.result.continuation_token = cpp_result.ContinuationToken.empty() ? nullptr : strdup(cpp_result.ContinuationToken.c_str());
        response.data.result.prefix = cpp_result.Prefix.empty() ? nullptr : strdup(cpp_result.Prefix.c_str());
    } else {
        response.is_error = true;
        const auto& cpp_error = result.error();
        response.data.error.code = strdup(cpp_error.Code.c_str());
        response.data.error.message = strdup(cpp_error.Message.c_str());
        response.data.error.resource = strdup(cpp_error.Resource.c_str());
        response.data.error.request_id = cpp_error.RequestId;
    }

    return response;
}

void s3_free_string(char* str) {
    if (str) {
        free(str);
    }
}

void s3_free_error(s3_error* error) {
    if (!error)
        return;
    s3_free_string(error->code);
    s3_free_string(error->message);
    s3_free_string(error->resource);
}

void s3_free_list_buckets_result(s3_list_buckets_result* result) {
    if (!result)
        return;

    for (size_t i = 0; i < result->bucket_count; ++i) {
        s3_free_string(result->buckets[i].bucket_arn);
        s3_free_string(result->buckets[i].bucket_region);
        s3_free_string(result->buckets[i].creation_date);
        s3_free_string(result->buckets[i].name);
    }
    free(result->buckets);

    s3_free_string(result->owner.display_name);
    s3_free_string(result->owner.id);
    s3_free_string(result->continuation_token);
    s3_free_string(result->prefix);
}

void s3_free_list_buckets_response(s3_list_buckets_response* response) {
    if (!response)
        return;
    if (response->is_error) {
        s3_free_error(&response->data.error);
    } else {
        s3_free_list_buckets_result(&response->data.result);
    }
}
}
