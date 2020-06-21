mod unit;
mod decoder;
mod encoder;

pub mod null;
pub use null::Null;
pub mod ffmpeg;

pub use unit::Unit;
pub use decoder::Decoder;
pub use encoder::Encoder;

#[derive(Clone, Copy)]
pub struct Config {
    pub bitrate: usize,
    pub fps: usize,
    pub gop: usize,
    pub width: usize,
    pub height: usize,
}