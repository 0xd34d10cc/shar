use anyhow::{anyhow, Result};
use bytes::{Buf, BufMut, Bytes, BytesMut};
use iced::image;

use crate::codec::{Decoder, Encoder};

// Codec which does not actually compress the data
pub struct Null;

pub struct Unit(Bytes);

impl super::Unit for Unit {
    fn from_packet(data: &[u8]) -> Self {
        Unit(Bytes::copy_from_slice(data))
    }

    fn is_idr(&self) -> bool {
        true
    }

    fn data(&self) -> &[u8] {
        &self.0
    }
}

impl Encoder for Null {
    // TODO: decouple from iced structures
    type Frame = image::Handle;
    type Unit = Unit;

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
                units.push(Unit(buffer.freeze()));
                Ok(())
            }
            data => Err(anyhow!("Unexpected frame data format: {:?}", data)),
        }
    }
}

impl Decoder for Null {
    type Frame = image::Handle;
    type Unit = Unit;

    fn decode(&mut self, mut unit: Unit, frames: &mut Vec<Self::Frame>) -> Result<()> {
        let width = unit.0.get_u32_le();
        let height = unit.0.get_u32_le();
        let pixels = unit.0.to_vec();
        let frame = image::Handle::from_pixels(width, height, pixels);
        frames.push(frame);
        Ok(())
    }
}
