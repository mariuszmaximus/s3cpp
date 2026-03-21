use std::ffi::CString;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn get_object(key: *const std::ffi::c_char) -> *const std::ffi::c_char {
    let result = format!("Hello from Rust! Key: {:?}", key).to_string();
    let c_result = CString::new(result).expect("CString::new failed");
    c_result.as_ptr()
}
