use ffmpeg_sys_next as ff;

pub struct Unit {
    inner: *mut ff::AVPacket,
}

unsafe impl Send for Unit {}

impl Drop for Unit {
    fn drop(&mut self) {
        unsafe {
            ff::av_packet_free(&mut self.inner);
        }
    }
}

impl crate::codec::Unit for Unit {
    fn from_packet(data: &[u8]) -> Self {
        Self::from_data(data)
    }

    fn is_idr(&self) -> bool {
        // FIXME: keyframe != idr
        unsafe { ((*self.inner).flags & ff::AV_PKT_FLAG_KEY) != 0 }
    }

    fn data(&self) -> &[u8] {
        Unit::data(self)
    }

    fn timestamp(&self) -> u64 {
        self.pts() as u64
    }
}

impl Unit {
    pub fn from_data(data: &[u8]) -> Self {
        // TODO: use av_packet_from_data
        unsafe {
            let buffer = ff::av_buffer_alloc(data.len() as i32);
            debug_assert!(!buffer.is_null());
            std::ptr::copy_nonoverlapping(data.as_ptr(), (*buffer).data, data.len());

            let unit = ff::av_packet_alloc();
            debug_assert!(!unit.is_null());
            (*unit).buf = buffer;
            (*unit).data = (*buffer).data;
            (*unit).size = (*buffer).size;

            Unit { inner: unit }
        }
    }

    pub fn empty() -> Self {
        unsafe {
            let unit = ff::av_packet_alloc();
            Unit { inner: unit }
        }
    }

    pub fn pts(&self) -> i64 {
        unsafe { (*self.inner).pts }
    }

    pub fn set_pts(&mut self, pts: i64) {
        unsafe {
            (*self.inner).pts = pts;
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut ff::AVPacket {
        self.inner
    }

    pub fn data(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts((*self.inner).data, (*self.inner).size as usize) }
    }
}
