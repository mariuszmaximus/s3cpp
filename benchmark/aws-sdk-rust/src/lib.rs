use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn get_object(key: *const c_char) -> *mut c_char {
    let key_str = if key.is_null() {
        "<null>".to_string()
    } else {
        unsafe { CStr::from_ptr(key) }.to_string_lossy().into_owned()
    };

    let result = format!("rust: Key: {}", key_str);
    CString::new(result).unwrap().into_raw()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn free_object(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            drop(CString::from_raw(ptr));
        }
    }
}
