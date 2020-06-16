use bytes::{Buf, BufMut, Bytes, BytesMut};
use iced::image;

pub fn encode(frame: image::Handle) -> BytesMut {
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
            buffer
        }
        data => panic!("Unexpected data format: {:?}", data),
    }
}

pub fn decode(mut buffer: Bytes) -> image::Handle {
    let width = buffer.get_u32_le();
    let height = buffer.get_u32_le();
    let pixels = buffer.to_vec();
    image::Handle::from_pixels(width, height, pixels)
}
