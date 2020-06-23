use ffmpeg_sys_next as ff;

fn luma(r: u8, g: u8, b: u8) -> u8 {
    (((66 * r as i32 + 129 * g as i32 + 25 * b as i32) >> 8) + 16) as u8
}

unsafe fn to_yuv420(
    bgra: &[u8],
    width: usize,
    height: usize,
) -> (Box<[u8]>, usize /* ysize */, usize /* uvsize */) {
    let ysize = width * height;
    let uvsize = width * height / 4;
    let mut buffer = vec![0; ysize + uvsize + uvsize].into_boxed_slice();

    let ys = buffer.as_mut_ptr();
    let us = buffer.as_mut_ptr().offset(ysize as isize);
    let vs = buffer
        .as_mut_ptr()
        .offset(ysize as isize)
        .offset(uvsize as isize);

    let mut i: usize = 0;
    let mut ui: usize = 0;

    for line in 0..height {
        if line & 2 == 0 {
            let mut x = 0;
            while x < width {
                let r = *bgra.get_unchecked(4 * i + 2);
                let g = *bgra.get_unchecked(4 * i + 1);
                let b = *bgra.get_unchecked(4 * i + 0);

                let y = luma(r, g, b);
                let u = (((-38 * r as i32 + -74 * g as i32 + 112 * b as i32) >> 8) + 128) as u8;
                let v = (((112 * r as i32 + -94 * g as i32 + -18 * b as i32) >> 8) + 128) as u8;

                *ys.offset(i as isize) = y;
                *us.offset(ui as isize) = u;
                *vs.offset(ui as isize) = v;

                i += 1;
                ui += 1;

                let r = *bgra.get_unchecked(4 * i + 2);
                let g = *bgra.get_unchecked(4 * i + 1);
                let b = *bgra.get_unchecked(4 * i + 0);

                let y = luma(r, g, b);

                *ys.offset(i as isize) = y;

                i += 1;

                x += 2;
            }
        } else {
            for _x in 0..width {
                let r = *bgra.get_unchecked(4 * i + 2);
                let g = *bgra.get_unchecked(4 * i + 1);
                let b = *bgra.get_unchecked(4 * i + 0);

                let y = luma(r, g, b);
                *ys.offset(i as isize) = y;

                i += 1;
            }
        }
    }

    (buffer, ysize, uvsize)
}

pub struct Frame {
    inner: *mut ff::AVFrame,

    // NOTE: this field is here only to control the lifetime of AVFrame undelying buffer
    #[allow(unused)]
    data: Option<Box<[u8]>>,
}

impl Drop for Frame {
    fn drop(&mut self) {
        unsafe {
            ff::av_frame_free(&mut self.inner);
        }
    }
}

impl Frame {
    pub fn from_bgra(data: &[u8], width: usize, height: usize) -> Self {
        unsafe {
            let (mut yuv, ysize, uvsize) = to_yuv420(data, width, height);

            //  TODO: use AVBuffer to allow sharing Frame
            let frame = ff::av_frame_alloc();
            debug_assert!(!frame.is_null());

            (*frame).format = ff::AVPixelFormat::AV_PIX_FMT_YUV420P as i32;
            (*frame).height = height as i32;
            (*frame).width = width as i32;

            (*frame).data[0] = yuv.as_mut_ptr();
            (*frame).data[1] = yuv.as_mut_ptr().offset(ysize as isize);
            (*frame).data[2] = yuv
                .as_mut_ptr()
                .offset(ysize as isize)
                .offset(uvsize as isize);

            (*frame).extended_data = &mut (*frame).data[0] as *mut _;

            (*frame).linesize[0] = width as i32;
            (*frame).linesize[1] = (width / 2) as i32;
            (*frame).linesize[2] = (width / 2) as i32;

            let divisor = num_integer::gcd(width, height);
            (*frame).sample_aspect_ratio.num = (width / divisor) as i32;
            (*frame).sample_aspect_ratio.den = (height / divisor) as i32;

            Frame {
                inner: frame,
                data: Some(yuv),
            }
        }
    }

    pub fn empty() -> Self {
        unsafe {
            let frame = ff::av_frame_alloc();
            debug_assert!(!frame.is_null());

            Frame {
                inner: frame,
                data: None,
            }
        }
    }

    pub fn channel(&self, index: usize) -> (*const u8, usize) {
        unsafe {
            let data = (*self.inner).data[index];
            let linesize = (*self.inner).linesize[index] as usize;

            (data, linesize)
        }
    }

    pub fn width(&self) -> usize {
        unsafe { (*self.inner).width as usize }
    }

    pub fn height(&self) -> usize {
        unsafe { (*self.inner).height as usize }
    }

    pub fn set_pts(&mut self, pts: usize) {
        unsafe {
            (*self.inner).pts = pts as i64;
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut ff::AVFrame {
        self.inner
    }
}
