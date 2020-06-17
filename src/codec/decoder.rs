use anyhow::Result;
use bytes::Bytes;

// TODO: the bounds are too strict for some generic cases
pub trait Decoder: Send + 'static {
    type Frame;

    fn decode(&mut self, packet: Bytes) -> Result<Vec<Self::Frame>>;
}

impl<D, F> Decoder for Box<D>
where
    D: Decoder<Frame = F> + ?Sized,
{
    type Frame = F;

    fn decode(&mut self, packet: Bytes) -> Result<Vec<Self::Frame>> {
        <D as Decoder>::decode(self, packet)
    }
}
