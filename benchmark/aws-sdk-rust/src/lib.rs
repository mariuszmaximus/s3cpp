use std::ffi::{CStr, CString, c_void};
use std::os::raw::c_char;

use aws_config::Region;
use aws_credential_types::Credentials;
use aws_sdk_s3::Config;

#[::tokio::main]
async fn initialize_client(access: String, secret: String, endpoint: String) -> aws_sdk_s3::Client {
    let credentials = Credentials::from_keys(access, secret, None);
    let config = Config::builder()
        .credentials_provider(credentials)
        .endpoint_url("http://127.0.0.1:9000/")
        .region(Region::new("eu-west-1"))
        .build();
    let s3_client = aws_sdk_s3::Client::from_conf(config);
    s3_client
}

#[::tokio::main]
async fn do_create_bucket(client: &aws_sdk_s3::Client, bucket: &str) {
    client
        .create_bucket()
        .bucket(bucket)
        .send()
        .await
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): create_bucket failed: {err}");
            std::process::exit(1);
        });
}

#[::tokio::main]
async fn do_put_object(client: &aws_sdk_s3::Client, bucket: &str, key: &str, contents: &str) {
    client
        .put_object()
        .bucket(bucket)
        .key(key)
        .body(contents.as_bytes().to_vec().into())
        .send()
        .await
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): put_object failed: {err}");
            std::process::exit(1);
        });
}

#[::tokio::main]
async fn do_get_object(client: &aws_sdk_s3::Client, bucket: &str, key: &str) -> String {
    let resp = client
        .get_object()
        .bucket(bucket)
        .key(key)
        .send()
        .await
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): get_object failed: {err}");
            std::process::exit(1);
        });

    let bytes = resp
        .body
        .collect()
        .await
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): failed to collect body: {err}");
            std::process::exit(1);
        })
        .into_bytes();

    String::from_utf8(bytes.to_vec()).unwrap_or_else(|err| {
        eprintln!("fatal(rust): object is not valid UTF-8: {err}");
        std::process::exit(1);
    })
}

// ABI

// Helper to convert from *const c_char to String
fn c_char_to_str(ch: *const c_char) -> String {
    unsafe { CStr::from_ptr(ch) }.to_string_lossy().into_owned()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn init_client(
    access: *const c_char,
    secret: *const c_char,
    endpoint: *const c_char,
) -> *mut c_void {
    let access_rs = c_char_to_str(access);
    let secret_rs = c_char_to_str(secret);
    let endpoint_rs = c_char_to_str(endpoint);

    let sdk_client = initialize_client(access_rs, secret_rs, endpoint_rs);

    Box::into_raw(Box::new(sdk_client)) as *mut core::ffi::c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn create_bucket(handle: *mut c_void, bucket: *const c_char) {
    let bucket_rs = c_char_to_str(bucket);
    let client = unsafe { &mut *(handle as *mut aws_sdk_s3::Client) };
    do_create_bucket(client, &bucket_rs);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn put_object(handle: *mut c_void, bucket: *const c_char, key: *const c_char, contents: *const c_char) {
    let bucket_rs = c_char_to_str(bucket);
    let key_rs = c_char_to_str(key);
    let contents_rs = c_char_to_str(contents);
    let client = unsafe { &mut *(handle as *mut aws_sdk_s3::Client) };
    do_put_object(client, &bucket_rs, &key_rs, &contents_rs);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn get_object(handle: *mut c_void, bucket: *const c_char, key: *const c_char) -> *mut c_char {
    let bucket_rs = c_char_to_str(bucket);
    let key_rs = c_char_to_str(key);
    let client = unsafe { &mut *(handle as *mut aws_sdk_s3::Client) };

    let contents = do_get_object(&client, &bucket_rs, &key_rs);

    CString::new(contents)
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): CString conversion failed: {err}");
            std::process::exit(1);
        })
        .into_raw()
}
