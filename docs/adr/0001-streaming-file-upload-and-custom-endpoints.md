# ADR 0001: Streaming File Uploads and Configurable S3 Endpoints

- Status: Accepted
- Date: 2026-06-12

## Context

`s3cpp` supports uploading an in-memory string, but it does not provide a
file-backed upload operation. Supporting file uploads without loading the
complete file into memory requires:

- upload progress reporting and cancellation;
- HTTP and HTTPS custom endpoints;
- a configurable AWS signing region;
- `Content-Type` and `Content-Length` metadata on uploaded objects.

The existing `PutObject` implementation accepts an in-memory string. This is
appropriate for small objects, but it causes memory usage to grow with the file
size and cannot report streaming upload progress.

AWS Signature Version 4 normally includes the SHA-256 hash of the complete
payload. A streamed file therefore needs its hash before the signed HTTP
request starts.

## Decision

Add a file-backed single-request `PutObjectFile` operation.

`PutObjectFile` performs these steps:

1. Read the file and calculate its SHA-256 hash with OpenSSL EVP.
2. Sign the request using the precomputed payload hash.
3. Stream the file through libcurl using `CURLOPT_UPLOAD`.
4. Report progress through libcurl's transfer progress callback.

The callback returns `true` to continue or `false` to cancel the transfer.

Add `HttpFileRequest` as a separate request type instead of adding file state
to `HttpBodyRequest`. This keeps in-memory and file-backed uploads explicit and
prevents accidental buffering of file contents.

Extend the custom-endpoint `S3Client` constructor with:

- `useHttps`, controlling the URL scheme;
- `region`, used by AWS Signature Version 4.

Keep the existing constructor as a compatibility overload. It defaults to HTTP
and the `us-east-1` region, matching the previous custom-endpoint behavior.

Apply optional `Content-Type` and `Content-Length` values in the existing
in-memory `PutObject` implementation as well.

## Consequences

### Positive

- File memory usage remains bounded regardless of object size.
- Applications can display upload progress and cancel an upload.
- The same client can connect to HTTP MinIO instances and HTTPS S3-compatible
  services.
- File uploads are signed with the actual payload hash.
- Existing users of the custom-endpoint constructor remain source compatible.

### Negative

- Each file is read twice: once for SHA-256 calculation and once for upload.
- Upload begins only after the initial hashing pass completes.
- The implementation uses a single S3 `PutObject` request and does not support
  multipart upload, retrying individual parts, or resuming an interrupted
  upload.
- A single `PutObject` is limited to 5 GiB by the S3 API.
- The new implementation adds direct OpenSSL EVP and filesystem usage.

## Alternatives Considered

### Load the complete file into a string

Rejected because memory consumption would scale with file size and progress
would only describe sending an already-buffered payload.

### Use unsigned payload mode

Rejected as the default because support differs between S3-compatible servers
and it weakens payload integrity guarantees.

### Implement multipart upload

Deferred. Multipart upload is the appropriate solution for large files,
per-part retries, and resumable transfers, but requires additional S3
operations and lifecycle handling. It should be introduced in a separate ADR.

## Scope

This decision covers single-request file uploads and endpoint configuration.
It does not define multipart upload, download streaming, retry policy, or
persistent transfer state.
