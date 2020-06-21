use anyhow::{anyhow, Result};
use bytes::{Buf, BufMut, Bytes, BytesMut};
use iced::image;

use crate::codec::{Decoder, Encoder};

// Codec which does not actually compress the data
pub struct Null;

impl Encoder for Null {
    // TODO: decouple from iced structures
    type Frame = image::Handle;
    type Unit = BytesMut;

    fn encode(&mut self, frame: Self::Frame, units: &mut Vec<Self::Unit>) -> Result<()> {
        use iced_native::image::Data;

        match frame.data() {
            Data::Pixels {
                width,
                height,
                pixels,
            } => {
                let mut buffer = BytesMut::with_capacity(pixels.len() + 8);
                buffer.put_u32_le(*width);
                buffer.put_u32_le(*height);
                buffer.extend(pixels);
                units.push(buffer);
                Ok(())
            }
            data => Err(anyhow!("Unexpected frame data format: {:?}", data)),
        }
    }
}

impl Decoder for Null {
    type Frame = image::Handle;

    // FIXME: use smallvec
    fn decode(&mut self, mut packet: Bytes) -> Result<Vec<Self::Frame>> {
        let width = packet.get_u32_le();
        let height = packet.get_u32_le();
        let pixels = packet.to_vec();
        let image = image::Handle::from_pixels(width, height, pixels);
        Ok(vec![image])
    }
}
