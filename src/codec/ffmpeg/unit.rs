use std::ops::Deref;

use ffmpeg_sys_next as ff;

pub struct Unit {
    inner: *mut ff::AVPacket,
}

unsafe impl Send for Unit {}

impl Drop for Unit {
    fn drop(&mut self) {
        unsafe {
            ff::av_packet_free(&mut self.inner);
        }
    }
}

impl Unit {
    pub fn from_data(data: &[u8]) -> Self {
        unsafe {
            let buffer = ff::av_buffer_alloc(data.len() as i32);
            debug_assert!(!buffer.is_null());
            std::ptr::copy_nonoverlapping(data.as_ptr(), (*buffer).data, data.len());

            let unit = ff::av_packet_alloc();
            debug_assert!(!unit.is_null());
            (*unit).buf = buffer;
            (*unit).data = (*buffer).data;
            (*unit).size = (*buffer).size;

            Unit {
                inner: unit
            }
        }
    }

    pub fn empty() -> Self {
        unsafe {
            let unit = ff::av_packet_alloc();
            Unit {
                inner: unit,
            }
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut ff::AVPacket {
        self.inner
    }
}

impl Deref for Unit {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        unsafe {
            std::slice::from_raw_parts((*self.inner).data, (*self.inner).size as usize)
        }
    }
}