use ffmpeg_sys_next as ff;

use crate::codec;
use super::codec::Codec;

pub struct Context {
    codec: Codec,
    ptr: *mut ff::AVCodecContext
}

unsafe impl Send for Context {}

impl Context {
    pub fn new(mut codec: Codec, config: codec::Config) -> Self {
        unsafe {
            let context = ff::avcodec_alloc_context3(codec.as_mut_ptr());
            debug_assert!(!context.is_null());

            (*context).bit_rate = config.bitrate as i64;
            (*context).time_base.num = 1;
            (*context).time_base.den = config.fps as i32;
            (*context).gop_size = config.gop as i32;
            (*context).pix_fmt = ff::AVPixelFormat::AV_PIX_FMT_YUV420P;

            (*context).width = config.width as i32;
            (*context).height = config.height as i32;
            // Support resolutions up to 4k
            (*context).max_pixels = 4096 * 2160;
            (*context).get_buffer2 = Some(ff::avcodec_default_get_buffer2);

            unsafe extern "C" fn get_format(_context: *mut ff::AVCodecContext, fmts: *const ff::AVPixelFormat) -> ff::AVPixelFormat {
                let mut p = fmts;
                while *p != ff::AVPixelFormat::AV_PIX_FMT_NONE {
                    if *p == ff::AVPixelFormat::AV_PIX_FMT_YUV420P {
                        return ff::AVPixelFormat::AV_PIX_FMT_YUV420P;
                    }
                    p = p.offset(1);
                }

                assert!(false);
                return ff::AVPixelFormat::AV_PIX_FMT_NONE;
            }

            (*context).get_format = Some(get_format);

            let divisor = num_integer::gcd(config.width, config.height);
            (*context).sample_aspect_ratio.num = (config.width / divisor) as i32;
            (*context).sample_aspect_ratio.den = (config.height / divisor) as i32;
            

            Context {
                codec,
                ptr: context
            }
        }
    }

    pub fn codec(&self) -> Codec {
        self.codec
    }

    pub fn as_mut_ptr(&mut self) -> *mut ff::AVCodecContext {
        self.ptr
    }
}

impl Drop for Context {
    fn drop(&mut self) {
        unsafe {
            ff::avcodec_free_context(&mut self.ptr);
        }
    }
}
