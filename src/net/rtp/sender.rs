use std::io;
use std::net::SocketAddr;
use std::time::Duration;

use futures::stream::{Stream, StreamExt};
use tokio::net::UdpSocket;

use super::packet::Packet;
use super::packetizer::Packetizer;
use crate::codec::Unit;

pub struct Sender<S> {
    units: S,

    sequence: u16,
}

impl<S, U> Sender<S>
where
    S: Stream<Item = U> + Unpin,
    U: Unit,
{
    pub fn new(units: S) -> Self {
        Sender { units, sequence: 0 }
    }

    pub async fn stream_to(&mut self, remote: SocketAddr) -> io::Result<()> {
        let local: SocketAddr = ([0, 0, 0, 0], 0).into();
        // ...why the heck are these async? -_-
        let mut socket = UdpSocket::bind(local).await?;
        socket.connect(remote).await?;

        let mut npackets = 0usize;

        let mut buffer = [0u8; 2048];
        while let Some(unit) = self.units.next().await {
            let data = unit.data();
            let mtu = 1400;
            let mut packetizer = Packetizer::with_mtu(mtu, data);

            while let Some(fragment) = packetizer.next() {
                const HEADER_SIZE: usize = 12;
                let packet_size = HEADER_SIZE + 2 + fragment.payload.len();
                let mut packet = Packet::new(&mut buffer[..packet_size]).unwrap();

                let header = packet.header_mut();
                header.set_version(2);
                header.set_payload_type(96);
                header.set_sequence(self.sequence);
                header.set_timestamp(unit.timestamp() as u32);

                let payload = packet.payload_mut();
                payload[0] = fragment.indicator;
                payload[1] = fragment.header;
                payload[2..].copy_from_slice(fragment.payload);

                self.sequence = self.sequence.wrapping_add(1);

                socket.send(&buffer[..packet_size]).await?;

                npackets += 1;
                if npackets % 128 == 0 {
                    tokio::time::delay_for(Duration::from_millis(1)).await;
                }
            }
        }

        Ok(())
    }
}
