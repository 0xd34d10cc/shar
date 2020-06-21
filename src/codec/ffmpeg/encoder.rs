use std::ffi::CStr;

use anyhow::{anyhow, Result};
use iced::image;
use iced_native::image::Data;
use ffmpeg_sys_next as ff;

use super::codec::Codec;
use super::context::Context;
use super::frame::Frame;
use super::unit::Unit;
use super::error::{Error, ErrorKind};
use crate::codec;


pub struct Encoder {
    context: Context,
    config: codec::Config,
    frame_index: usize,
}

impl Encoder {
    pub fn new(config: codec::Config) -> Result<Self> {
        let priority_list = [
            //b"h264_nvenc\0"
        ];

        let mut codec = None;
        for &name in priority_list.iter() {
            let name = CStr::from_bytes_with_nul(name).unwrap();
            if let Some(c) = Codec::find_encoder(name) {
                log::info!("Using {} encoder", name.to_string_lossy());
                codec = Some(c);
                break;
            }
        }

        // NOTE: this might fail if ffmpeg was built without x264
        let mut codec = codec
            .or_else(Codec::default_encoder)
            .ok_or_else(|| anyhow!("Could not find any ffmpeg encoders"))?;

        let mut context = Context::new(codec, config);

        // TODO: allow to customize
        let mut opts: *mut ff::AVDictionary = std::ptr::null_mut();
        let code = unsafe {
            ff::avcodec_open2(context.as_mut_ptr(), codec.as_mut_ptr(), &mut opts)
        };
        if code < 0 {
            return Err(Error::from_code(code).into());
        }

        Ok(Encoder {
            context,
            config,
            frame_index: 0,
        })
    }

    pub fn encode(&mut self, mut frame: Frame, units: &mut Vec<Unit>) -> Result<()> {
        // TODO: support dynamic resize
        // let size_changed = self.config.width != frame.width() || self.config.height != frame.height

        let pts = self.next_pts();
        frame.set_pts(pts);

        unsafe {
            let code = ff::avcodec_send_frame(self.context.as_mut_ptr(), frame.as_mut_ptr());
            if code != 0 {
                return Err(Error::from_code(code).into());
            }

            let mut unit = Unit::empty();
            loop {
                let code = ff::avcodec_receive_packet(self.context.as_mut_ptr(), unit.as_mut_ptr());
                let error = Error::from_code(code);
                match error.kind() {
                    ErrorKind::Success => {
                        let len = unit.data().len();
                        // NOTE: the underlying memory of |unit| could be reused by codec, so we
                        //       have to do memcpy here
                        let cpy = Unit::from_data(unit.data());
                        units.push(cpy);
                        unit = Unit::empty();
                    },
                    ErrorKind::WouldBlock => break,
                    _ => return Err(error.into()),
                }
            }
        }
        Ok(())
    }

    fn next_pts(&mut self) -> usize {
        // Clock rate (number of ticks in 1 second) for H264 video. (RFC 6184 Section 8.2.1)
        const CLOCK_RATE: usize = 90000;

        let ticks_per_frame = CLOCK_RATE / self.config.fps;
        let pts = ticks_per_frame * self.frame_index;
        self.frame_index += 1;
        pts
    }
}

impl codec::Encoder for Encoder {
    type Frame = image::Handle;
    type Unit = Unit;

    fn encode(&mut self, frame: Self::Frame, units: &mut Vec<Self::Unit>) -> Result<()> {
        // TODO: move bgra -> yuv420 conversion to a separate task
        let frame = match frame.data() {
            Data::Pixels {
                pixels,
                width,
                height,
            } => Frame::from_bgra(pixels, *width as usize, *height as usize),
            data => panic!("Unexpected frame format: {:?}", data),
        };

        self.encode(frame, units)
    }
}
