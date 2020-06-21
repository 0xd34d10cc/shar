mod decoder;
mod encoder;
mod null;
pub mod ffmpeg;

pub use decoder::Decoder;
pub use encoder::Encoder;
pub use null::Null;

#[derive(Clone, Copy)]
pub struct Config {
    pub bitrate: usize,
    pub fps: usize,
    pub gop: usize,
    pub width: usize,
    pub height: usize,
}