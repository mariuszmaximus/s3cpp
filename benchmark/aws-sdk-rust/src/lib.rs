use std::ffi::{CStr, CString, c_void};
use std::os::raw::c_char;

use aws_config::Region;
use aws_credential_types::Credentials;
use aws_sdk_s3::Config;

struct ClientState {
    client: aws_sdk_s3::Client,
    runtime: tokio::runtime::Runtime,
}

fn initialize_client(access: String, secret: String, endpoint: String) -> ClientState {
    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): failed to create tokio runtime: {err}");
            std::process::exit(1);
        });

    let credentials = Credentials::from_keys(access, secret, None);
    let config = Config::builder()
        .credentials_provider(credentials)
        .endpoint_url("http://127.0.0.1:9000/")
        .region(Region::new("eu-west-1"))
        .build();
    let client = aws_sdk_s3::Client::from_conf(config);

    ClientState { client, runtime }
}

fn do_create_bucket(state: &ClientState, bucket: &str) {
    state.runtime.block_on(async {
        state.client
            .create_bucket()
            .bucket(bucket)
            .send()
            .await
            .unwrap_or_else(|err| {
                eprintln!("fatal(rust): create_bucket failed: {err}");
                std::process::exit(1);
            });
    });
}

fn do_put_object(state: &ClientState, bucket: &str, key: &str, contents: &str) {
    state.runtime.block_on(async {
        state.client
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
    });
}

fn do_get_object(state: &ClientState, bucket: &str, key: &str) -> String {
    state.runtime.block_on(async {
        let resp = state.client
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

    let state = initialize_client(access_rs, secret_rs, endpoint_rs);

    Box::into_raw(Box::new(state)) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn create_bucket(handle: *mut c_void, bucket: *const c_char) {
    let bucket_rs = c_char_to_str(bucket);
    let state = unsafe { &*(handle as *mut ClientState) };
    do_create_bucket(state, &bucket_rs);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn put_object(handle: *mut c_void, bucket: *const c_char, key: *const c_char, contents: *const c_char) {
    let bucket_rs = c_char_to_str(bucket);
    let key_rs = c_char_to_str(key);
    let contents_rs = c_char_to_str(contents);
    let state = unsafe { &*(handle as *mut ClientState) };
    do_put_object(state, &bucket_rs, &key_rs, &contents_rs);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn get_object(handle: *mut c_void, bucket: *const c_char, key: *const c_char) -> *mut c_char {
    let bucket_rs = c_char_to_str(bucket);
    let key_rs = c_char_to_str(key);
    let state = unsafe { &*(handle as *mut ClientState) };

    let contents = do_get_object(state, &bucket_rs, &key_rs);

    CString::new(contents)
        .unwrap_or_else(|err| {
            eprintln!("fatal(rust): CString conversion failed: {err}");
            std::process::exit(1);
        })
        .into_raw()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn free_client(handle: *mut c_void) {
    drop(unsafe { Box::from_raw(handle as *mut ClientState) });
}
