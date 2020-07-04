use std::ffi::CStr;

use ffmpeg_sys_next as ff;

#[derive(Clone, Copy)]
pub struct Codec(*mut ff::AVCodec);

// The codec is basically reference to a immutable static variable
unsafe impl Send for Codec {}
unsafe impl Sync for Codec {}

impl Codec {
    pub fn name(&self) -> &CStr {
        unsafe {
            CStr::from_ptr((*self.0).name)
        }
    }

    pub fn default_encoder() -> Option<Self> {
        unsafe {
            let ptr = ff::avcodec_find_encoder(ff::AVCodecID::AV_CODEC_ID_H264);
            Self::from_ptr(ptr)
        }
    }

    pub fn find_encoder(name: &CStr) -> Option<Self> {
        unsafe {
            let ptr = ff::avcodec_find_encoder_by_name(name.as_ptr());
            Self::from_ptr(ptr)
        }
    }

    pub fn default_decoder() -> Option<Self> {
        unsafe {
            let ptr = ff::avcodec_find_decoder(ff::AVCodecID::AV_CODEC_ID_H264);
            Self::from_ptr(ptr)
        }
    }

    pub fn find_decoder(name: &CStr) -> Option<Self> {
        unsafe {
            let ptr = ff::avcodec_find_decoder_by_name(name.as_ptr());
            Self::from_ptr(ptr)
        }
    }

    unsafe fn from_ptr(ptr: *mut ff::AVCodec) -> Option<Self> {
        if ptr.is_null() {
            None
        } else {
            Some(Codec(ptr))
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut ff::AVCodec {
        self.0
    }
}
