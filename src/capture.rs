use std::hash::{Hash, Hasher};
use std::time::Duration;
use std::io::ErrorKind;

use anyhow::{anyhow, Result};
use scrap::{Capturer, Display};
use iced::Subscription;
use iced::image;
use iced_native::subscription::Recipe;
use tokio::sync::mpsc::{self, error::TrySendError};
use futures::stream::{BoxStream, StreamExt};


// fn capture(display: Display) -> Result<mpsc::Receiver<image::Handle>>  {
//     let (sender, receiver) = mpsc::channel(10);
//     let capturer = Capturer::new(display).context("Unable to create display capturer")?;

//     std::thread::spawn(|| {

//     })

//     receiver
// }


pub fn capture(id: DisplayID, fps: u32) -> Subscription<image::Handle> {
    Subscription::from_recipe(CaptureDisplay {
        id,
        fps,
    })
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum DisplayID {
    Primary,
    Index(usize)
}

#[derive(Debug, Clone)]
struct CaptureDisplay {
    id: DisplayID,
    fps: u32,
}

impl CaptureDisplay {
    fn spawn_capturer(&self) -> Result<mpsc::Receiver<image::Handle>> {
        let (mut sender, receiver) = mpsc::channel(10);

        let config = self.clone();
        std::thread::spawn(move || {
            let display = config.display().unwrap();
            let mut capturer = Capturer::new(display).unwrap();
            let period = Duration::from_secs(1) / config.fps;

            loop {
                let pixels = match capturer.frame() {
                    Ok(frame) => frame.to_vec(),
                    Err(e) if e.kind() == ErrorKind::WouldBlock => {
                        std::thread::sleep(Duration::from_millis(1));
                        continue;
                    },
                    Err(e) => panic!("Failed to capture the frame: {}", e),
                };

                let width = capturer.width() as u32;
                let height = capturer.height() as u32;
                let handle = image::Handle::from_pixels(width, height, pixels);
                if let Err(TrySendError::Closed(_)) = sender.try_send(handle) {
                    break;
                }

                std::thread::sleep(period);
            }
        });

        Ok(receiver)
    }

    fn display(&self) -> Result<Display> {
        match self.id {
            DisplayID::Primary => {
                let display = Display::primary()?;
                Ok(display)
            },
            DisplayID::Index(i) => {
                let displays = Display::all().unwrap();
                let display = displays.into_iter().skip(i).next().ok_or_else(|| {
                    anyhow!("Display index {} is out of range", i)
                })?;
                Ok(display)
            }
        }
    }
}


impl<H, I> Recipe<H, I> for CaptureDisplay where H: Hasher {
    type Output = image::Handle;

    fn hash(&self, state: &mut H) {
        self.id.hash(state);
        self.fps.hash(state);
    }

    fn stream(self: Box<Self>, _input: BoxStream<'static, I>) -> BoxStream<'static, Self::Output> {
        let receiver = self.spawn_capturer().unwrap();
        receiver.boxed()
    }
}
