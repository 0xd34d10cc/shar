use anyhow::Result;

use super::Unit;

pub trait Encoder: Send + 'static {
    type Frame: Send + 'static;
    type Unit: Unit;

    fn encode(&mut self, frame: Self::Frame, units: &mut Vec<Self::Unit>) -> Result<()>;
}

impl<E, F, U> Encoder for Box<E>
where
    E: Encoder<Frame = F, Unit = U> + ?Sized,
    F: Send + 'static,
    U: Unit,
{
    type Frame = F;
    type Unit = U;

    fn encode(&mut self, frame: Self::Frame, units: &mut Vec<Self::Unit>) -> Result<()> {
        <E as Encoder>::encode(self, frame, units)
    }
}
