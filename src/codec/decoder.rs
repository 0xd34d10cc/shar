use anyhow::Result;

use super::Unit;

// TODO: the bounds are too strict for some generic cases
pub trait Decoder: Send + 'static {
    type Frame;
    type Unit: Unit;

    fn decode(&mut self, unit: Self::Unit, frames: &mut Vec<Self::Frame>) -> Result<()>;
}

impl<D, F, U> Decoder for Box<D>
where
    D: Decoder<Frame = F, Unit = U> + ?Sized,
    U: Unit,
{
    type Frame = F;
    type Unit = U;

    fn decode(&mut self, unit: U, frames: &mut Vec<Self::Frame>) -> Result<()> {
        <D as Decoder>::decode(self, unit, frames)
    }
}
