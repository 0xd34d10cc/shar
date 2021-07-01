#![allow(unused)]

use std::net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr};

use byteorder::NetworkEndian;
use zerocopy::{AsBytes, ByteSlice, ByteSliceMut, FromBytes, LayoutVerified, Unaligned, U16, U32};

const MAGIC: [u8; 4] = [0x21, 0x12, 0xA4, 0x42];

pub const MAGIC_COOKIE: u32 = u32::from_be_bytes(MAGIC);

// STUN message header has the following format:
//
//   0               1               2               3
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |0 0|   STUN Message Type     |       Message Length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                        Magic Cookie                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                                                               |
//  |             Transaction ID(96 bits, 12 bytes)                 |
//  |                                                               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// RFC 5389 Section 6
#[derive(Debug, FromBytes, AsBytes, Unaligned)]
#[repr(C, packed)]
pub struct Header {
    type_: U16<NetworkEndian>,
    len: U16<NetworkEndian>,
    cookie: U32<NetworkEndian>,
    id: [u8; 12],
}

impl Header {
    pub fn id(&self) -> &[u8; 12] {
        &self.id
    }

    pub fn class(&self) -> u8 {
        let msb = self.type_.as_bytes()[0];
        let lsb = self.type_.as_bytes()[1];
        (msb & 1) << 1 | (lsb & (1 << 4)) >> 4
    }

    pub fn method(&self) -> u16 {
        let msb = self.type_.as_bytes()[0] as u16;
        let lsb = self.type_.as_bytes()[1] as u16;
        (msb & 0b00111110) << 7 | (lsb & 0b11100000) >> 1 | (lsb & 0b00001111)
    }
}

pub struct Packet<B> {
    header: LayoutVerified<B, Header>,
    payload: B,
}

impl<B> Packet<B>
where
    B: ByteSlice + Copy,
{
    pub fn parse(bytes: B) -> Option<Packet<B>> {
        let (header, payload) = LayoutVerified::<_, Header>::new_from_prefix(bytes)?;
        if header.cookie.as_bytes() != MAGIC {
            return None;
        }

        Some(Packet { header, payload })
    }

    pub fn header(&self) -> &Header {
        &self.header
    }

    pub fn payload(&self) -> B {
        self.payload
    }

    pub fn attributes(&self) -> impl Iterator<Item = Attribute<B>>
    where
        B: Default,
    {
        Attributes { data: self.payload }
    }
}

impl<B> Packet<B>
where
    B: ByteSliceMut,
{
    pub fn new(bytes: B) -> Option<Packet<B>> {
        let (header, payload) = LayoutVerified::new_from_prefix_zeroed(bytes)?;
        Some(Packet { header, payload })
    }

    pub fn header_mut(&mut self) -> &mut Header {
        &mut self.header
    }

    pub fn payload_mut(&mut self) -> &mut [u8] {
        &mut self.payload
    }
}

pub fn bind_request(id: [u8; 12]) -> [u8; 20] {
    let mut packet = [0u8; 20];
    let mut request = Packet::new(&mut packet[..]).unwrap();
    request.header_mut().type_.set(0x1); // request
    request.header_mut().len.set(0);
    request.header_mut().cookie.set(MAGIC_COOKIE);
    request.header_mut().id = id;
    packet
}

#[derive(FromBytes, AsBytes, Unaligned)]
#[repr(C, packed)]
struct AttributeHeader {
    type_: U16<NetworkEndian>,
    len: U16<NetworkEndian>,
}

struct Attributes<B> {
    data: B,
}

impl<B> Iterator for Attributes<B>
where
    B: ByteSlice + Default,
{
    type Item = Attribute<B>;

    fn next(&mut self) -> Option<Self::Item> {
        let data = std::mem::take(&mut self.data);
        let (header, data) = LayoutVerified::<_, AttributeHeader>::new_from_prefix(data)?;
        let len = header.len.get() as usize;
        if len > data.len() {
            return None;
        }

        let (data, rest) = data.split_at(len);
        self.data = rest;

        match header.type_.get() {
            0x01 /* MAPPED-ADDRESS */ => {
                let family = data.get(1)?;
                match family {
                    0x01 /* IPv4 */ => {
                        if data.len() != 8 {
                            return None;
                        }

                        let port = u16::from_be_bytes([data[2], data[3]]);
                        let ip = Ipv4Addr::new(data[4], data[5], data[6], data[7]);
                        Some(Attribute::MappedAddress((IpAddr::V4(ip), port).into()))
                    },
                    0x02 /* IPv6 */ => {
                        if data.len() != 20 {
                            return None;
                        }

                        let port = u16::from_be_bytes([data[2], data[3]]);
                        let ip = [
                            data[4],  data[5],  data[6],  data[7],
                            data[8],  data[9],  data[10], data[11],
                            data[12], data[13], data[14], data[15],
                            data[16], data[17], data[18], data[19]
                        ];
                        let ip = Ipv6Addr::from(ip);
                        Some(Attribute::MappedAddress((IpAddr::V6(ip), port).into()))
                    },
                    _ => None,
                }
            }
            0x20 /* XOR-MAPPED_ADDRESS */ => {
                let family = data.get(1)?;
                match family {
                    0x01 /* IPv4 */ => {
                        if data.len() != 8 {
                            return None;
                        }

                        let port = u16::from_be_bytes([data[2] ^ MAGIC[0], data[3] ^ MAGIC[1]]);
                        let ip = Ipv4Addr::new(
                            data[4] ^ MAGIC[0],
                            data[5] ^ MAGIC[1],
                            data[6] ^ MAGIC[2],
                            data[7] ^ MAGIC[3]
                        );
                        Some(Attribute::XorMappedIpAddr((IpAddr::V4(ip), port).into()))
                    },
                    0x02 /* IPv6 */ => {
                        if data.len() != 20 {
                            return None;
                        }

                        let port = u16::from_be_bytes([data[2] ^ MAGIC[0], data[3] ^ MAGIC[1]]);
                        let ip = [
                            data[4]  ^ MAGIC[0], data[5]  ^ MAGIC[1], data[6]  ^ MAGIC[2], data[7]  ^ MAGIC[3],
                            data[8]  ^ MAGIC[0], data[9]  ^ MAGIC[1], data[10] ^ MAGIC[2], data[11] ^ MAGIC[3],
                            data[12] ^ MAGIC[0], data[13] ^ MAGIC[1], data[14] ^ MAGIC[2], data[15] ^ MAGIC[3],
                            data[16] ^ MAGIC[0], data[17] ^ MAGIC[1], data[18] ^ MAGIC[2], data[19] ^ MAGIC[3]
                        ];
                        let ip = Ipv6Addr::from(ip);
                        Some(Attribute::XorMappedIpAddr((IpAddr::V6(ip), port).into()))
                    },
                    _ => None,
                }
            }
            type_ => Some(Attribute::Unknown(type_, data)),
        }
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum Attribute<B> {
    Unknown(u16, B),
    MappedAddress(SocketAddr),
    XorMappedIpAddr(SocketAddr),
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deserialize() {
        const BYTES: &[u8] = b"\x01\x01\x00\x0c\
                               \x21\x12\xa4\x42\
                               \x4e\x58\x8f\x99\
                               \x8f\x37\x3e\x3a\
                               \x4e\xb8\x9f\x65\
                               \x00\x20\x00\x08\
                               \x00\x01\x8c\x8e\
                               \xd3\x4d\x10\xcc";

        let packet = Packet::parse(BYTES).unwrap();
        assert_eq!(packet.header().type_.get(), 0x0101);
        assert_eq!(packet.header().len.get(), 12);
        assert_eq!(packet.header().cookie.get(), 0x2112a442);
        assert_eq!(
            packet.header().id,
            [0x4e, 0x58, 0x8f, 0x99, 0x8f, 0x37, 0x3e, 0x3a, 0x4e, 0xb8, 0x9f, 0x65]
        );

        let mut attributes = packet.attributes();
        assert_eq!(
            attributes.next(),
            Some(Attribute::XorMappedIpAddr(
                "242.95.180.142:44444".parse().unwrap()
            ))
        );
    }

    #[allow(unused)]
    fn example_request() {
        use std::net::{ToSocketAddrs, UdpSocket};

        let addr = "stun.l.google.com:19302"
            .to_socket_addrs()
            .unwrap()
            .next()
            .unwrap();
        let s = UdpSocket::bind("0.0.0.0:2500").unwrap();
        let packet = bind_request([1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4]);

        s.send_to(&packet, addr).unwrap();
        let mut packet = [0u8; 256];
        let (n, addr) = s.recv_from(&mut packet).unwrap();
        eprintln!("Response from: {}", addr);
        let response = Packet::parse(&packet[..n]).unwrap();
        eprintln!("{:?}", response.header());
        eprintln!("{:?}", response.payload());
        eprintln!("{:?}", response.attributes().collect::<Vec<_>>());
    }
}
