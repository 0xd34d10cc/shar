use anyhow::Result;
use bytes::BytesMut;

pub trait Encoder: Send + 'static {
    type Frame;

    fn encode(&mut self, frame: Self::Frame) -> Result<BytesMut>;
}

impl<E, F> Encoder for Box<E> where E: Encoder<Frame=F> + ?Sized {
    type Frame = F;

    fn encode(&mut self, frame: Self::Frame) -> Result<BytesMut> {
        <E as Encoder>::encode(self, frame)
    }
}
