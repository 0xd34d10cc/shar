use std::ffi::CStr;

use super::codec::Codec;
use super::context::Context;
use crate::codec;

pub struct Decoder {
    context: Context,
}

impl Decoder {
    pub fn new(config: codec::Config) -> Option<Self> {
        let priority_list = [
            b"h264_cuvid\0"
        ];

        let mut codec = None;
        for &name in priority_list.iter() {
            let name = CStr::from_bytes_with_nul(name).unwrap();
            if let Some(c) = Codec::find_decoder(name) {
                codec = Some(c);
                break;
            }
        }

        // NOTE: this should never fail, because avcodec includes software h264 decdoer
        let codec = codec.or_else(Codec::default_decoder)?;
        let context = Context::new(codec, config);
        Some(Decoder { context })
    }
}
