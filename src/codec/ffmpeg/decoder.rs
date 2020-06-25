use std::ffi::CStr;

use anyhow::{anyhow, Result};
use ffmpeg_sys_next as ff;
use iced::image;

use super::codec::Codec;
use super::context::Context;
use super::error::{Error, ErrorKind};
use super::frame::Frame;
use super::unit::Unit;
use crate::codec;

pub struct Decoder {
    context: Context,
}

impl Decoder {
    pub fn new(config: codec::Config) -> Result<Self> {
        let priority_list = [
            // b"h264_cuvid\0" // doesn't support AV_PIX_FMT_YUV420P
        ];

        let mut codec = None;
        for &name in priority_list.iter() {
            let name = CStr::from_bytes_with_nul(name).unwrap();
            if let Some(c) = Codec::find_decoder(name) {
                log::info!("Using {} decoder", name.to_string_lossy());
                codec = Some(c);
                break;
            }
        }

        // NOTE: this should never fail, because avcodec includes software h264 decdoer
        let mut codec = codec
            .or_else(Codec::default_decoder)
            .ok_or_else(|| anyhow!("Could not find any ffmpeg decoders"))?;
        let mut context = Context::new(codec, config);

        let mut opts: *mut ff::AVDictionary = std::ptr::null_mut();
        let code =
            unsafe { ff::avcodec_open2(context.as_mut_ptr(), codec.as_mut_ptr(), &mut opts) };

        if code < 0 {
            return Err(Error::from_code(code).into());
        }

        Ok(Decoder { context })
    }

    pub fn decode(&mut self, mut unit: Unit, frames: &mut Vec<Frame>) -> Result<()> {
        unsafe {
            let code = ff::avcodec_send_packet(self.context.as_mut_ptr(), unit.as_mut_ptr());
            if code != 0 {
                // FIXME: handle abscence of I-frames correctly
                return Ok(());
                // return Err(Error::from_code(code).into());
            }

            let mut frame = Frame::empty();
            loop {
                let code = ff::avcodec_receive_frame(self.context.as_mut_ptr(), frame.as_mut_ptr());
                let error = Error::from_code(code);
                match error.kind() {
                    ErrorKind::Success => {
                        frames.push(frame);
                        frame = Frame::empty();
                    }
                    ErrorKind::WouldBlock => break,
                    _ => return Err(error.into()),
                }
            }
            Ok(())
        }
    }
}

#[inline(always)]
fn as_byte(x: i32) -> u8 {
    if x > 255 {
        return 255;
    }

    if x < 0 {
        return 0;
    }

    return x as u8;
}

fn to_bgra(frame: Frame) -> (usize, usize, Vec<u8>) {
    let (ys, yline) = frame.channel(0);
    let (us, uline) = frame.channel(1);
    let (vs, vline) = frame.channel(2);
    let width = frame.width();
    let height = frame.height();

    let mut data: Vec<u8> = Vec::with_capacity(width * height * 4);

    let yi = |x, y| (y * yline + x) as isize;
    let ui = |x, y| ((y / 2) * uline + x / 2) as isize;
    let vi = |x, y| ((y / 2) * vline + x / 2) as isize;

    unsafe {
        let dst = data.as_mut_ptr();
        let mut i = 0;
        for line in 0..height {
            for col in 0..width {
                let y = *ys.offset(yi(col, line));
                let u = *us.offset(ui(col, line));
                let v = *vs.offset(vi(col, line));

                let c = y as i32 - 16;
                let d = u as i32 - 128;
                let e = v as i32 - 128;

                let r = as_byte((298 * c + 409 * e + 128) >> 8);
                let g = as_byte((298 * c - 100 * d - 208 * e + 128) >> 8);
                let b = as_byte((298 * c + 516 * d + 128) >> 8);
                let a = 0xff;

                *dst.offset(i + 0) = b;
                *dst.offset(i + 1) = g;
                *dst.offset(i + 2) = r;
                *dst.offset(i + 3) = a;

                i += 4;
            }
        }

        data.set_len(width * height * 4);
    }

    (width, height, data)
}

impl codec::Decoder for Decoder {
    type Frame = image::Handle;
    type Unit = Unit;

    fn decode(&mut self, unit: Unit, frames: &mut Vec<Self::Frame>) -> Result<()> {
        // TODO: move yuv -> bgra conversion to a separate task
        let mut ff_frames = Vec::new();
        self.decode(unit, &mut ff_frames)?;
        for frame in ff_frames {
            // convert from yuv to bgra
            // convert to Handle
            let (width, height, pixels) = to_bgra(frame);
            frames.push(image::Handle::from_pixels(
                width as u32,
                height as u32,
                pixels,
            ));
        }
        Ok(())
    }
}
