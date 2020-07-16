const NRI: u8 = 0b01100000;
const PACKET_TYPE: u8 = 0b00011111;

#[derive(Clone, Copy)]
pub struct Indicator(u8);

impl Indicator {
    pub fn nri(&self) -> u8 {
        (self.0 & NRI) >> NRI.trailing_zeros()
    }

    pub fn packet_type(&self) -> u8 {
        (self.0 & PACKET_TYPE) >> PACKET_TYPE.trailing_zeros()
    }
}

const START: u8 = 0b10000000;
const END: u8 = 0b01000000;
const NAL_TYPE: u8 = 0b00011111;

#[derive(Clone, Copy)]
pub struct Header(u8);

impl Header {
    pub fn is_first(&self) -> bool {
        (self.0 & START) != 0
    }

    pub fn is_last(&self) -> bool {
        (self.0 & END) != 0
    }

    pub fn nal_type(&self) -> u8 {
        (self.0 & NAL_TYPE) >> NAL_TYPE.trailing_zeros()
    }
}

pub struct Fragment<'a> {
    data: &'a [u8],
}

impl Fragment<'_> {
    pub fn parse(data: &[u8]) -> Option<Fragment> {
        if data.len() < 2 {
            return None;
        }

        Some(Fragment { data })
    }

    pub fn payload(&self) -> &[u8] {
        &self.data[2..]
    }

    pub fn indicator(&self) -> Indicator {
        Indicator(self.data[0])
    }

    pub fn header(&self) -> Header {
        Header(self.data[1])
    }
}
