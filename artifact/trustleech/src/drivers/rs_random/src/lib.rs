#![no_std]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // Halt or reset the system, log panic info, etc.
    loop {}
}

use core::sync::atomic::{AtomicUsize, Ordering};
use rand_core::Error;

fn getrandom_inner(dest: &mut [u8]) -> Result<(), Error> {
    static COUNTER: AtomicUsize = AtomicUsize::new(0);
    for chunk in dest.chunks_mut(4) {
        let value = COUNTER.fetch_add(1, Ordering::Relaxed).to_ne_bytes();
        for (c, v) in chunk.iter_mut().zip(value.iter()) {
            *c = *v;
        }
    }
    Ok(())
}

pub fn getrandom(dest: &mut [u8]) -> Result<(), Error> {
    getrandom_inner(dest)
}
