const PACKET_TYPE_FU_A: u8 = 28;
const NRI_MASK: u8 = 0b0110_0000;
const NAL_TYPE_MASK: u8 = 0b0001_1111;

pub struct Packetizer<'a> {
    data: &'a [u8],
    mtu: usize,

    nri: u8,
    nal_type: u8,

    remaining: usize,
    last_packet_was_full_unit: bool,
}

// FU-A fragment:
//
// 	0               1               2               3
// 	7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	| FU indicator  |   FU header   |                             |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                             |
// 	|                                                             |
// 	|                         FU payload                          |
// 	|                                                             |
// 	|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 	|                               :...OPTIONAL RTP padding      |
// 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
#[derive(Debug, PartialEq, Eq)]
pub struct Fragment<'a> {
    // FU indicator:
    //
    //	+---------------+
    // 	|7|6|5|4|3|2|1|0|
    // 	+-+-+-+-+-+-+-+-+
    // 	|F|NRI|   PT    |
    // 	+---------------+
    //
    pub indicator: u8,

    // FU header:
    //
    //	+---------------+
    // 	|7|6|5|4|3|2|1|0|
    // 	+-+-+-+-+-+-+-+-+
    // 	|S|E|R|  Type   |
    // 	+---------------+
    //
    pub header: u8,

    pub payload: &'a [u8],
}

impl<'a> Packetizer<'a> {
    pub fn with_mtu(mtu: usize, data: &'a [u8]) -> Packetizer<'a> {
        Packetizer {
            data,
            mtu,

            nri: 0,
            nal_type: 0,

            remaining: 0,
            last_packet_was_full_unit: false,
        }
    }

    // move to next nal unit, set |nri|, |nal_type| and |remaining|
    fn next_unit(&mut self) {
        let header_index = match self.data {
            [0x00, 0x00, 0x01, ..] => 3,
            [0x00, 0x00, 0x00, 0x01, ..] => 4,
            _ => {
                debug_assert!(false);
                return;
            }
        };

        self.nri = self.data[header_index] & NRI_MASK;
        self.nal_type = self.data[header_index] & NAL_TYPE_MASK;
        // self.data.advance(header_index + 1);
        self.data = &self.data[header_index + 1..]; // drop pattern and header

        // find start of next unit
        // TODO: use memchr3?
        if let Some(pos) = self.data.windows(3).position(|s| s == [0x00, 0x00, 0x01]) {
            self.remaining = pos;
            if pos != 0 && self.data[pos - 1] == 0 {
                self.remaining -= 1;
            }
        } else {
            self.remaining = self.data.len();
        }
    }

    pub fn next(&mut self) -> Option<Fragment<'a>> {
        if self.last_packet_was_full_unit {
            self.last_packet_was_full_unit = false;
            let indicator = self.nri | PACKET_TYPE_FU_A;
            let header = (1 << 6) | self.nal_type;
            let payload = b"";
            return Some(Fragment {
                indicator,
                header,
                payload,
            });
        }

        let mut first = 0;
        let mut last = 0;

        if self.remaining == 0 {
            if self.data.is_empty() {
                return None;
            }

            self.next_unit();
            first = 1;
        }

        let size = std::cmp::min(self.remaining, self.mtu);
        self.remaining -= size;

        if self.remaining == 0 {
            if first == 1 {
                // from RFC6184: Start bit and End bit MUST NOT both be set
                //               to one in the same FU header
                // ... so we have to generate entire (though empty) packet just for end flag
                self.last_packet_was_full_unit = true;
            } else {
                last = 1;
            }
        }

        let indicator = self.nri | PACKET_TYPE_FU_A;
        let header = (first << 7) | (last << 6) | self.nal_type;
        // let payload = self.data.split_to(size);
        let (payload, data) = self.data.split_at(size);
        self.data = data;
        Some(Fragment {
            indicator,
            header,
            payload,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const FIRST: u8 = 1 << 7;
    const LAST: u8 = 1 << 6;

    #[test]
    fn empty_data() {
        let mut packetizer = Packetizer::with_mtu(20, b"");
        assert_eq!(packetizer.next(), None);
    }

    #[test]
    fn packetize_full_units() {
        let mut data = [
            0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00,
            0x80, 0x0c, 0x32, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
        ];

        let mut packetizer = Packetizer::with_mtu(20, &mut data);

        // first nal unit
        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A,
                header: FIRST | 9,
                payload: &[0x10]
            })
        );

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A,
                header: LAST | 9,
                payload: &[]
            })
        );

        // second unit
        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: FIRST | 7,
                payload: &[0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32]
            })
        );

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: LAST | 7,
                payload: &[]
            })
        );

        // third and last
        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: FIRST | 8,
                payload: &[0xce, 0x3c, 0x80]
            })
        );

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: LAST | 8,
                payload: &[]
            })
        );

        assert_eq!(packetizer.next(), None);
    }

    #[test]
    fn packetize_partial_units() {
        let data = [
            0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
        ];

        let mut packetizer = Packetizer::with_mtu(3, &data);

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: FIRST | 7,
                payload: &[0x42, 0x00, 0x20]
            })
        );

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: 7,
                payload: &[0xe9, 0x00, 0x80]
            })
        );

        assert_eq!(
            packetizer.next(),
            Some(Fragment {
                indicator: PACKET_TYPE_FU_A | (3 << 5),
                header: LAST | 7,
                payload: &[0x0c, 0x32]
            })
        );
    }
}
