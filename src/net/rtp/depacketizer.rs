use super::fragment::Fragment;

pub struct Depacketizer {
    buffer: Vec<u8>,
    complete: bool,
}

impl Depacketizer {
    pub fn new() -> Self {
        Depacketizer {
            buffer: Vec::with_capacity(4096),
            complete: false,
        }
    }

    pub fn push(&mut self, fragment: Fragment) {
        if fragment.header().is_first() {
            // setup nal unit prefix

            if self.buffer.is_empty() {
                self.buffer.extend_from_slice(&[0x00, 0x00, 0x00, 0x01]);
            } else {
                self.buffer.extend_from_slice(&[0x00, 0x00, 0x01]);
            }

            // recover nal header
            let nri = fragment.indicator().nri() << 5;
            let nt = fragment.header().nal_type();
            self.buffer.push(nri | nt);
        }

        // push actual data
        self.buffer.extend_from_slice(fragment.payload());

        // from RFC 6184: Start bit and End bit MUST NOT both be set
        //                to one in the same FU header
        debug_assert!(!fragment.header().is_first() || !fragment.header().is_last());
        self.complete = fragment.header().is_last();
    }

    pub fn clear(&mut self) {
        self.buffer.clear();
        self.complete = false;
    }

    pub fn complete(&self) -> bool {
        self.complete
    }

    pub fn bytes(&self) -> &[u8] {
        &self.buffer
    }
}
