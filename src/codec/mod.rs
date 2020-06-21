mod decoder;
mod encoder;
mod unit;

pub mod null;
pub use null::Null;
pub mod ffmpeg;

pub use decoder::Decoder;
pub use encoder::Encoder;
pub use unit::Unit;

#[derive(Clone, Copy)]
pub struct Config {
    pub bitrate: usize,
    pub fps: usize,
    pub gop: usize,
    pub width: usize,
    pub height: usize,
}
