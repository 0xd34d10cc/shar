use std::fmt;
use std::hash::{Hash, Hasher};
use std::io::ErrorKind;
use std::time::Duration;

use anyhow::{anyhow, Result};
use futures::channel::mpsc;
use futures::stream::{BoxStream, StreamExt};
use iced::{image, Subscription};
use scrap::{Capturer, Display};

pub fn capture(id: DisplayId, fps: u32) -> Subscription<image::Handle> {
    Subscription::from_recipe(CaptureDisplay { id, fps })
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum DisplayId {
    Primary,
    Index(usize),
}

impl fmt::Display for DisplayId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DisplayId::Primary => write!(f, "Primary display"),
            DisplayId::Index(i) => write!(f, "Display #{}", i),
        }
    }
}

#[derive(Debug, Clone, Hash)]
pub struct CaptureDisplay {
    pub id: DisplayId,
    pub fps: u32,
}

impl CaptureDisplay {
    pub fn start(&self) -> Result<mpsc::Receiver<image::Handle>> {
        let (mut sender, receiver) = mpsc::channel(5);

        let config = self.clone();
        std::thread::spawn(move || {
            let display = config.display().unwrap();
            let mut capturer = Capturer::new(display).unwrap();
            let period = Duration::from_secs(1) / config.fps;

            loop {
                let mut pixels = match capturer.frame() {
                    Ok(frame) => frame.to_vec(),
                    Err(e) if e.kind() == ErrorKind::WouldBlock => {
                        std::thread::sleep(Duration::from_millis(1));
                        continue;
                    }
                    Err(e) => panic!("Failed to capture the frame: {}", e),
                };

                let width = capturer.width() as u32;
                let height = capturer.height() as u32;

                // set alpha to 1
                let len = pixels.len();
                let mut i = 3;
                while i < len {
                    pixels[i] = 0xff;
                    i += 4;
                }

                let handle = image::Handle::from_pixels(width, height, pixels);

                match sender.try_send(handle) {
                    Ok(_) => {}
                    Err(e) if e.is_disconnected() => break,
                    Err(e) if e.is_full() => {
                        log::warn!("Captured frame drops because consumer could not keep up");
                    }
                    Err(e) => {
                        log::error!("Failed to send frame: {}", e);
                    }
                }

                std::thread::sleep(period);
            }
        });

        Ok(receiver)
    }

    fn display(&self) -> Result<Display> {
        match self.id {
            DisplayId::Primary => {
                let display = Display::primary()?;
                Ok(display)
            }
            DisplayId::Index(i) => {
                let displays = Display::all().unwrap();
                let display = displays
                    .into_iter()
                    .nth(i)
                    .ok_or_else(|| anyhow!("Display index {} is out of range", i))?;
                Ok(display)
            }
        }
    }
}

impl<H, I> iced_native::subscription::Recipe<H, I> for CaptureDisplay
where
    H: Hasher,
{
    type Output = image::Handle;

    fn hash(&self, state: &mut H) {
        <Self as Hash>::hash(self, state);
    }

    fn stream(self: Box<Self>, _input: BoxStream<'static, I>) -> BoxStream<'static, Self::Output> {
        let receiver = self.start().unwrap();
        receiver.boxed()
    }
}
