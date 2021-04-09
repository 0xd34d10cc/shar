use std::io;
use std::marker::PhantomData;
use std::net::SocketAddr;

use futures::sink::{Sink, SinkExt};
use tokio::net::UdpSocket;

use super::depacketizer::Depacketizer;
use super::fragment::Fragment;
use super::packet::Packet;
use crate::codec::Unit;

pub struct Receiver<S, U> {
    sink: S,
    _unit: PhantomData<U>,
}

impl<S, U> Receiver<S, U>
where
    S: Sink<U> + Unpin,
    U: Unit,
{
    pub fn new(sink: S) -> Self {
        Receiver {
            sink,
            _unit: PhantomData,
        }
    }

    pub async fn receive(&mut self, address: SocketAddr) -> io::Result<()> {
        let socket = UdpSocket::bind(address).await?;

        const MAX_MTU: usize = 2048; // TODO: unhardcode
        let mut buffer = [0u8; MAX_MTU];

        let mut depacketizer = Depacketizer::new();
        let mut stream_source = None;
        let mut sequence = 0;
        let mut timestamp = 0;
        let mut drop = false;

        loop {
            let (n, sender) = socket.recv_from(&mut buffer).await?;

            match stream_source {
                Some(current_sender) => {
                    if current_sender != sender {
                        log::warn!(
                            "Changing stream source from {} to {}",
                            current_sender,
                            sender
                        );
                        drop = true;
                        stream_source = Some(sender);
                    }
                }
                None => {
                    log::info!("Receiving stream from: {}", sender);
                    stream_source = Some(sender);
                }
            }

            let packet = Packet::parse(&buffer[..n]).unwrap();
            let fragment = Fragment::parse(packet.payload()).unwrap(); // FIXME

            let in_sequence = packet.header().sequence() == sequence + 1;
            if !in_sequence && !drop {
                drop = true;
                log::info!(
                    "Dropped a packet. Packet type: {}, NAL type: {}",
                    fragment.indicator().packet_type(),
                    fragment.header().nal_type()
                );
            }

            let flush = timestamp != packet.header().timestamp();
            if flush {
                if depacketizer.complete() {
                    let unit = U::from_packet(depacketizer.bytes());
                    if self.sink.send(unit).await.is_err() {
                        log::info!("Closed successfully");
                        return Ok(());
                    }
                }

                timestamp = packet.header().timestamp();
                depacketizer.clear();
            }

            let accept = if drop {
                fragment.header().is_first()
            } else {
                in_sequence
            };

            if !accept {
                continue;
            }

            if drop {
                log::info!(
                    "Recovered from drop. Packet type: {}. NAL type: {}",
                    fragment.indicator().packet_type(),
                    fragment.header().nal_type()
                );
                depacketizer.clear();
            }

            drop = false;
            sequence = packet.header().sequence();
            depacketizer.push(fragment);
        }
    }
}
