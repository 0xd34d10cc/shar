use byteorder::NetworkEndian;
use zerocopy::{AsBytes, ByteSlice, ByteSliceMut, FromBytes, LayoutVerified, Unaligned, U16, U32};

//   0               1               2               3
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           timestamp                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |           synchronization source (SSRC) identifier            |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |            contributing source (CSRC) identifiers             |
//  |                             ....                              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The first twelve octets are present in every RTP packet, while the
// list of CSRC identifiers is present only when inserted by a mixer.
#[derive(FromBytes, AsBytes, Unaligned)]
#[repr(C)]
pub struct Header {
    flags: u8,
    payload_type: u8, // and marker
    pub sequence: U16<NetworkEndian>,
    pub timestamp: U32<NetworkEndian>,
    pub ssrc: U32<NetworkEndian>,
    // contributing_source: U32<NetworkEndian>
}

impl Header {
    pub fn version(&self) -> u8 {
        self.flags >> 6
    }

    pub fn set_version(&mut self, version: u8) {
        debug_assert!(version < 4);
        self.flags |= version << 6;
    }

    // TODO: use bitflags?
    pub fn has_padding(&self) -> bool {
        (self.flags & (1 << 5)) != 0
    }

    pub fn set_padding(&mut self, pad: bool) {
        if pad {
            self.flags |= 1 << 5;
        } else {
            self.flags &= !(1 << 5);
        }
    }

    pub fn has_extensions(&self) -> bool {
        (self.flags & (1 << 4)) != 0
    }

    pub fn set_extensions(&mut self, ext: bool) {
        if ext {
            self.flags |= 1 << 4;
        } else {
            self.flags &= !(1 << 4);
        }
    }

    pub fn contributors_count(&self) -> u8 {
        self.flags & 0b1111
    }

    pub fn marked(&self) -> bool {
        (self.payload_type & (1 << 7)) != 0
    }

    pub fn payload_type(&self) -> u8 {
        self.payload_type & !(1 << 7)
    }

    pub fn set_payload_type(&mut self, t: u8) {
        debug_assert!(t < 128);
        self.payload_type = (self.payload_type & (1 << 7)) | t;
    }

    pub fn sequence(&self) -> u16 {
        self.sequence.get()
    }

    pub fn set_sequence(&mut self, seq: u16) {
        self.sequence.set(seq)
    }

    pub fn timestamp(&self) -> u32 {
        self.timestamp.get()
    }

    pub fn set_timestamp(&mut self, t: u32) {
        self.timestamp.set(t);
    }

    pub fn ssrc(&self) -> u32 {
        self.ssrc.get()
    }

    pub fn set_ssrc(&mut self, ssrc: u32) {
        self.ssrc.set(ssrc);
    }
}

pub struct Packet<B> {
    header: LayoutVerified<B, Header>,
    payload: B,
}

impl<B> Packet<B>
where
    B: ByteSlice,
{
    pub fn parse(bytes: B) -> Option<Packet<B>> {
        let (header, payload) = LayoutVerified::new_from_prefix(bytes)?;
        Some(Packet { header, payload })
    }

    pub fn header(&self) -> &Header {
        &self.header
    }

    pub fn payload(&self) -> &B {
        &self.payload
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

    pub fn payload_mut(&mut self) -> &mut B {
        &mut self.payload
    }
}

#[cfg(test)]
mod tests {
    use super::Packet;

    #[test]
    fn parse_packet() {
        let data = b"\x80\x60\x32\x3a\x03\x4a\
            \x34\xc5\xd0\x1b\x17\xc4\x7c\x05\x97\x82\xcf\x39\xfc\xf3\xa8\xcf\
            \x68\xa9\x3c\x54\xbd\x6a\xd0\x75\x6e\x41\x2b\xf0\x97\x33\x00\x2f\
            \x8a\x34\xd6\x95\x82\x04\xbb\x4f\x08\xeb\x81\x31\x25\x13\xd0\x95\
            \xa6\x0e\x1e\xa8\x3c\x54\xd0\xc7\x89\xb0\xf8\x2b\x01\x1a\x56\xa4\
            \x03\xcc\xa3\x89\x6e\x10\xd4\x5d\xba\x2a\x8d\x49\xee\x67\x9d\x4f\
            \xf4\xb5\x8b\x9d\x18\xd1\xe6\xc2\x49\x8c\xca\x2b\x20\x2f\x1e\xf7\
            \x1d\xa1\xa3\x1a\x02\xe7\xcd\x39\xdd\x8a\xba\x49\xe0\xbd\xb9\x55\
            \x6f\x2a\x15\x9b\x1d\xed\x65\x4e\x8b\x7f\x6e\xe5\x47\xba\x2e\xd5\
            \xab\xef\xd8\x9b\x47\xef\x57\xe3\x3f\xb4\x01\x96\x80\xc6\x9f\x6c";

        let packet = Packet::parse(&data[..]).unwrap();
        let header = packet.header();
        assert_eq!(header.version(), 2);
        assert_eq!(header.has_padding(), false);
        assert_eq!(header.has_extensions(), false);
        assert_eq!(header.contributors_count(), 0);
        assert_eq!(header.marked(), false);
        assert_eq!(header.payload_type(), 96);
        assert_eq!(header.sequence(), 12858);
        assert_eq!(header.timestamp(), 55194821);
        assert_eq!(header.ssrc(), 0xd01b17c4);

        assert_eq!(&packet.payload()[..4], &[0x7c, 0x05, 0x97, 0x82][..]);
    }

    #[test]
    fn set_fields() {
        let mut data = [0u8; 256];
        let mut packet = Packet::new(&mut data[..]).unwrap();

        {
        let header = packet.header_mut();
        header.set_version(2);
        header.set_payload_type(42);
        header.set_sequence(1337);
        header.set_timestamp(45678);
        header.set_ssrc(0xd34d10cc);
        packet.payload_mut()[..4].copy_from_slice(&[0xd3, 0x3d, 0x10, 0xcc]);
        }

        let header = packet.header();
        assert_eq!(header.version(), 2);
        assert_eq!(header.has_padding(), false);
        assert_eq!(header.has_extensions(), false);
        assert_eq!(header.contributors_count(), 0);
        assert_eq!(header.marked(), false);
        assert_eq!(header.sequence(), 1337);
        assert_eq!(header.timestamp(), 45678);
        assert_eq!(header.ssrc(), 0xd34d10cc);
        assert_eq!(&packet.payload()[..4], &[0xd3, 0x3d, 0x10, 0xcc]);
    }
}
