#![no_std]
#![no_main]

extern crate alloc;

mod exceptions;
mod hal;
mod logger;
use arm_pl011_uart as uart;

use lazy_static::lazy_static;

use arm_pl011_uart::{DataBits, LineConfig, PL011Registers, Parity, StopBits};
use buddy_system_allocator::{Heap, LockedHeap};
use core::{
    mem::size_of,
    panic::PanicInfo,
    ptr::{self, null, null_mut, NonNull},
};
use flat_device_tree::{node::FdtNode, standard_nodes::Compatible, Fdt};
use hal::HalImpl;
use log::{debug, error, info, trace, warn, LevelFilter};
use safe_mmio::UniqueMmioPointer;
use smccc::{psci::system_off, Hvc};
use spin::mutex::{SpinMutex, SpinMutexGuard};
use uart::Uart;
use virtio_drivers::{
    device::{
        blk::VirtIOBlk,
        console::VirtIOConsole,
        gpu::VirtIOGpu,
        rng::VirtIORng,
        socket::{
            VirtIOSocket, VsockAddr, VsockConnectionManager, VsockEventType, VMADDR_CID_HOST,
        },
    },
    transport::{
        mmio::{MmioTransport, VirtIOHeader},
        pci::{
            bus::{
                BarInfo, Cam, Command, ConfigurationAccess, DeviceFunction, MemoryBarType, MmioCam,
                PciRoot,
            },
            virtio_device_type, PciTransport,
        },
        DeviceType, Transport,
    },
};

use alloc::{borrow::ToOwned, boxed, rc::Rc, vec, vec::Vec};
use core::{cell::RefCell, str::FromStr};

use smoltcp::iface::{Config, Interface, SocketHandle, SocketSet};
use smoltcp::phy::{Device, DeviceCapabilities, Medium, RxToken, TxToken};
use smoltcp::wire::{EthernetAddress, IpAddress, IpCidr, Ipv4Address};
use smoltcp::{socket::tcp, time::Duration, time::Instant};
use virtio_drivers::device::net::{RxBuffer, VirtIONet};
use virtio_drivers::Error;

use core::arch::asm;
use core::slice;
use core::str;

/// Base memory-mapped address of the primary PL011 UART device.
pub const UART_BASE_ADDRESS: *mut PL011Registers = 0x900_0000 as _;

#[global_allocator]
static HEAP_ALLOCATOR: LockedHeap<32> = LockedHeap::new();

static HEAP: SpinMutex<[u8; 0x1000000]> = SpinMutex::new([0; 0x1000000]);

#[inline(always)]
fn read_cntvct() -> u64 {
    let cntvct: u64;
    unsafe {
        asm!(
            "mrs {cntvct}, CNTVCT_EL0",
            cntvct = out(reg) cntvct,
        );
    }
    cntvct
}

#[inline(always)]
fn read_cntfrq() -> u64 {
    let cntfrq: u64;
    unsafe {
        asm!(
            "mrs {cntfrq}, CNTFRQ_EL0",
            cntfrq = out(reg) cntfrq,
        );
    }
    cntfrq
}

fn time_in_microseconds() -> i64 {
    let counter_value = read_cntvct();
    let counter_frequency = read_cntfrq();

    // Convert counter ticks to microseconds
    ((counter_value * 1_000_000) / counter_frequency) as i64
}

#[no_mangle]
extern "C" fn device_enumerate(x0: u64, x1: u64, x2: u64, x3: u64) {
    // Safe because UART_BASE_ADDRESS is the base of the MMIO region for a UART and is mapped as
    // device memory.
    #[cfg_attr(platform = "crosvm", allow(unused_mut))]
    let mut uart =
        Uart::new(unsafe { UniqueMmioPointer::new(NonNull::new(UART_BASE_ADDRESS).unwrap()) });
    uart.enable(
        LineConfig {
            data_bits: DataBits::Bits8,
            parity: Parity::None,
            stop_bits: StopBits::One,
        },
        115200,
        50000000,
    )
    .unwrap();
    logger::init(uart, LevelFilter::Off).unwrap();

    info!("virtio-drivers example started.");
    debug!(
        "x0={:#018x}, x1={:#018x}, x2={:#018x}, x3={:#018x}",
        x0, x1, x2, x3
    );

    // Give the allocator some memory to allocate.
    add_to_heap(
        &mut HEAP_ALLOCATOR.lock(),
        SpinMutexGuard::leak(HEAP.try_lock().unwrap()).as_mut_slice(),
    );

    info!("Loading FDT from {:#018x}", x0);
    // Safe because the pointer is a valid pointer to unaliased memory.
    let fdt = unsafe { Fdt::from_ptr(x0 as *const u8).unwrap() };

    for node in fdt.all_nodes() {
        // Dump information about the node for debugging.
        trace!(
            "{}: {:?}",
            node.name,
            node.compatible().map(Compatible::first),
        );
        for range in node.reg() {
            trace!(
                "  {:#018x?}, length {:?}",
                range.starting_address,
                range.size
            );
        }

        // Check whether it is a VirtIO MMIO device.
        if let (Some(compatible), Some(region)) = (node.compatible(), node.reg().next()) {
            if compatible.all().any(|s| s == "virtio,mmio")
                && region.size.unwrap_or(0) > size_of::<VirtIOHeader>()
            {
                debug!("Found VirtIO MMIO device at {:?}", region);

                let header = NonNull::new(region.starting_address as *mut VirtIOHeader).unwrap();
                match unsafe { MmioTransport::new(header, region.size.unwrap()) } {
                    Err(e) => warn!("Error creating VirtIO MMIO transport: {}", e),
                    Ok(transport) => {
                        info!(
                            "Detected virtio MMIO device with vendor id {:#X}, device type {:?}, version {:?}",
                            transport.vendor_id(),
                            transport.device_type(),
                            transport.version(),
                        );
                        if transport.device_type() == DeviceType::Network {
                            let mut tcp_mmio = TCP_MMIO_TRANSPORT.lock();
                            *tcp_mmio = Some(transport);
                        } else {
                            virtio_device(transport);
                        }
                    }
                }
            }
        }
    }

    if let Some(pci_node) = fdt.find_compatible(&["pci-host-cam-generic"]) {
        info!("Found PCI node: {}", pci_node.name);
        enumerate_pci(pci_node, Cam::MmioCam);
    }
    if let Some(pcie_node) = fdt.find_compatible(&["pci-host-ecam-generic"]) {
        info!("Found PCIe node: {}", pcie_node.name);
        enumerate_pci(pcie_node, Cam::Ecam);
    }
}

/// Adds the given memory range to the given heap.
fn add_to_heap<const ORDER: usize>(heap: &mut Heap<ORDER>, range: &'static mut [u8]) {
    // SAFETY: The range we pass is valid because it comes from a mutable static reference, which it
    // effectively takes ownership of.
    unsafe {
        heap.init(range.as_mut_ptr() as usize, range.len());
    }
}

fn virtio_device(transport: impl Transport) {
    match transport.device_type() {
        DeviceType::Block => virtio_blk(transport),
        DeviceType::GPU => virtio_gpu(transport),
        DeviceType::Network => virtio_net(transport),
        DeviceType::Console => virtio_console(transport),
        DeviceType::Socket => match virtio_socket(transport) {
            Ok(()) => info!("virtio-socket test finished successfully"),
            Err(e) => error!("virtio-socket test finished with error '{e:?}'"),
        },
        DeviceType::EntropySource => virtio_rng(transport),
        t => warn!("Unrecognized virtio device: {:?}", t),
    }
}

fn virtio_rng<T: Transport>(transport: T) {
    let mut bytes = [0u8; 8];
    let mut rng = VirtIORng::<HalImpl, T>::new(transport).expect("failed to create rng driver");
    let len = rng
        .request_entropy(&mut bytes)
        .expect("failed to receive entropy");
    info!("received {len} random bytes: {:?}", &bytes[..len]);
    info!("virtio-rng test finished");
}

fn virtio_blk<T: Transport>(transport: T) {
    let mut blk = VirtIOBlk::<HalImpl, T>::new(transport).expect("failed to create blk driver");
    assert!(!blk.readonly());
    let mut input = [0xffu8; 512];
    let mut output = [0; 512];
    for i in 0..32 {
        for x in input.iter_mut() {
            *x = i as u8;
        }
        blk.write_blocks(i, &input).expect("failed to write");
        blk.read_blocks(i, &mut output).expect("failed to read");
        assert_eq!(input, output);
    }
    info!("virtio-blk test finished");
}

fn virtio_gpu<T: Transport>(transport: T) {
    let mut gpu = VirtIOGpu::<HalImpl, T>::new(transport).expect("failed to create gpu driver");
    let (width, height) = gpu.resolution().expect("failed to get resolution");
    let width = width as usize;
    let height = height as usize;
    info!("GPU resolution is {}x{}", width, height);
    let fb = gpu.setup_framebuffer().expect("failed to get fb");
    for y in 0..height {
        for x in 0..width {
            let idx = (y * width + x) * 4;
            fb[idx] = x as u8;
            fb[idx + 1] = y as u8;
            fb[idx + 2] = (x + y) as u8;
        }
    }
    gpu.flush().expect("failed to flush");
    //delay some time
    info!("virtio-gpu show graphics....");
    for _ in 0..1000 {
        for _ in 0..100000 {
            unsafe {
                core::arch::asm!("nop");
            }
        }
    }

    info!("virtio-gpu test finished");
}

fn virtio_net<T: Transport>(transport: T) {
    info!("Starting TCP test!\n");
    const NET_BUFFER_LEN: usize = 4096 * 4;
    let net = virtio_drivers::device::net::VirtIONet::<HalImpl, T, NET_QUEUE_SIZE>::new(
        transport,
        NET_BUFFER_LEN,
    )
    .expect("failed to create net driver");
    info!("MAC address: {:02x?}", net.mac_address());
    // setup_tcp_handle(net);
    info!("Done!\n");

    info!("virtio-net test finished");
}

fn virtio_console<T: Transport>(transport: T) {
    let mut console =
        VirtIOConsole::<HalImpl, T>::new(transport).expect("Failed to create console driver");
    if let Some(size) = console.size().unwrap() {
        info!("VirtIO console {}", size);
    }
    for &c in b"Hello world on console!\n" {
        console.send(c).expect("Failed to send character");
    }
    let c = console.recv(true).expect("Failed to read from console");
    info!("Read {:?}", c);
    info!("virtio-console test finished");
}

fn virtio_socket<T: Transport>(transport: T) -> virtio_drivers::Result<()> {
    let mut socket = VsockConnectionManager::new(
        VirtIOSocket::<HalImpl, T>::new(transport).expect("Failed to create socket driver"),
    );
    let port = 1221;
    let host_address = VsockAddr {
        cid: VMADDR_CID_HOST,
        port,
    };
    info!("Connecting to host on port {port}...");
    socket.connect(host_address, port)?;
    let event = socket.wait_for_event()?;
    assert_eq!(event.source, host_address);
    assert_eq!(event.destination.port, port);
    assert_eq!(event.event_type, VsockEventType::Connected);
    info!("Connected to the host");

    const EXCHANGE_NUM: usize = 2;
    let messages = ["0-Ack. Hello from guest.", "1-Ack. Received again."];
    for k in 0..EXCHANGE_NUM {
        let mut buffer = [0u8; 24];
        let socket_event = socket.wait_for_event()?;
        let VsockEventType::Received { length, .. } = socket_event.event_type else {
            panic!("Received unexpected socket event {:?}", socket_event);
        };
        let read_length = socket.recv(host_address, port, &mut buffer)?;
        assert_eq!(length, read_length);
        info!(
            "Received message: {:?}({:?}), len: {:?}",
            buffer,
            core::str::from_utf8(&buffer[..length]),
            length
        );

        let message = messages[k % messages.len()];
        socket.send(host_address, port, message.as_bytes())?;
        info!("Sent message: {:?}", message);
    }
    socket.shutdown(host_address, port)?;
    info!("Shutdown the connection");
    Ok(())
}

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
enum PciRangeType {
    ConfigurationSpace,
    IoSpace,
    Memory32,
    Memory64,
}

impl From<u32> for PciRangeType {
    fn from(value: u32) -> Self {
        match value {
            0 => Self::ConfigurationSpace,
            1 => Self::IoSpace,
            2 => Self::Memory32,
            3 => Self::Memory64,
            _ => panic!("Tried to convert invalid range type {}", value),
        }
    }
}

fn enumerate_pci(pci_node: FdtNode, cam: Cam) {
    let reg = pci_node.reg();
    let mut allocator = PciMemory32Allocator::for_pci_ranges(&pci_node);

    for region in reg {
        info!(
            "Reg: {:?}-{:#x}",
            region.starting_address,
            region.starting_address as usize + region.size.unwrap()
        );
        assert_eq!(region.size.unwrap(), cam.size() as usize);
        // SAFETY: We know the pointer is to a valid MMIO region.
        let mut pci_root =
            PciRoot::new(unsafe { MmioCam::new(region.starting_address as *mut u8, cam) });
        for (device_function, info) in pci_root.enumerate_bus(0) {
            let (status, command) = pci_root.get_status_command(device_function);
            info!(
                "Found {} at {}, status {:?} command {:?}",
                info, device_function, status, command
            );
            if let Some(virtio_type) = virtio_device_type(&info) {
                info!("  VirtIO {:?}", virtio_type);
                allocate_bars(&mut pci_root, device_function, &mut allocator);
                dump_bar_contents(&mut pci_root, device_function, 4);
                let mut transport =
                    PciTransport::new::<HalImpl, _>(&mut pci_root, device_function).unwrap();
                info!(
                    "Detected virtio PCI device with device type {:?}, features {:#018x}",
                    transport.device_type(),
                    transport.read_device_features(),
                );
                if transport.device_type() == DeviceType::Network {
                    let mut tcp_pci = TCP_PCI_TRANSPORT.lock();
                    *tcp_pci = Some(transport);
                } else {
                    virtio_device(transport);
                }
            }
        }
    }
}

/// Allocates 32-bit memory addresses for PCI BARs.
struct PciMemory32Allocator {
    start: u32,
    end: u32,
}

impl PciMemory32Allocator {
    /// Creates a new allocator based on the ranges property of the given PCI node.
    pub fn for_pci_ranges(pci_node: &FdtNode) -> Self {
        let mut memory_32_address = 0;
        let mut memory_32_size = 0;
        for range in pci_node.ranges() {
            let prefetchable = range.child_bus_address_hi & 0x4000_0000 != 0;
            let range_type = PciRangeType::from((range.child_bus_address_hi & 0x0300_0000) >> 24);
            let bus_address = range.child_bus_address as u64;
            let cpu_physical = range.parent_bus_address as u64;
            let size = range.size as u64;
            info!(
                "range: {:?} {}prefetchable bus address: {:#018x} host physical address: {:#018x} size: {:#018x}",
                range_type,
                if prefetchable { "" } else { "non-" },
                bus_address,
                cpu_physical,
                size,
            );
            // Use the largest range within the 32-bit address space for 32-bit memory, even if it
            // is marked as a 64-bit range. This is necessary because crosvm doesn't currently
            // provide any 32-bit ranges.
            if !prefetchable
                && matches!(range_type, PciRangeType::Memory32 | PciRangeType::Memory64)
                && size > memory_32_size.into()
                && bus_address + size < u32::MAX.into()
            {
                assert_eq!(bus_address, cpu_physical);
                memory_32_address = u32::try_from(cpu_physical).unwrap();
                memory_32_size = u32::try_from(size).unwrap();
            }
        }
        if memory_32_size == 0 {
            panic!("No 32-bit PCI memory region found.");
        }
        Self {
            start: memory_32_address,
            end: memory_32_address + memory_32_size,
        }
    }

    /// Allocates a 32-bit memory address region for a PCI BAR of the given power-of-2 size.
    ///
    /// It will have alignment matching the size. The size must be a power of 2.
    pub fn allocate_memory_32(&mut self, size: u32) -> u32 {
        assert!(size.is_power_of_two());
        let allocated_address = align_up(self.start, size);
        assert!(allocated_address + size <= self.end);
        self.start = allocated_address + size;
        allocated_address
    }
}

const fn align_up(value: u32, alignment: u32) -> u32 {
    ((value - 1) | (alignment - 1)) + 1
}

fn dump_bar_contents(
    root: &mut PciRoot<impl ConfigurationAccess>,
    device_function: DeviceFunction,
    bar_index: u8,
) {
    let bar_info = root.bar_info(device_function, bar_index).unwrap();
    trace!("Dumping bar {}: {:#x?}", bar_index, bar_info);
    if let BarInfo::Memory { address, size, .. } = bar_info {
        let start = address as *const u8;
        unsafe {
            let mut buf = [0u8; 32];
            for i in 0..size / 32 {
                let ptr = start.add(i as usize * 32);
                ptr::copy(ptr, buf.as_mut_ptr(), 32);
                if buf.iter().any(|b| *b != 0xff) {
                    trace!("  {:?}: {:x?}", ptr, buf);
                }
            }
        }
    }
    trace!("End of dump");
}

/// Allocates appropriately-sized memory regions and assigns them to the device's BARs.
fn allocate_bars(
    root: &mut PciRoot<impl ConfigurationAccess>,
    device_function: DeviceFunction,
    allocator: &mut PciMemory32Allocator,
) {
    for (bar_index, info) in root.bars(device_function).unwrap().into_iter().enumerate() {
        let Some(info) = info else { continue };
        debug!("BAR {}: {}", bar_index, info);
        // Ignore I/O bars, as they aren't required for the VirtIO driver.
        if let BarInfo::Memory {
            address_type, size, ..
        } = info
        {
            // For now, only attempt to allocate 32-bit memory regions.
            if size > u32::MAX.into() {
                warn!("Skipping BAR {} with size {:#x}", bar_index, size);
                continue;
            }
            let size = size as u32;

            match address_type {
                MemoryBarType::Width32 => {
                    if size > 0 {
                        let address = allocator.allocate_memory_32(size);
                        debug!("Allocated address {:#010x}", address);
                        root.set_bar_32(device_function, bar_index as u8, address);
                    }
                }
                MemoryBarType::Width64 => {
                    if size > 0 {
                        let address = allocator.allocate_memory_32(size);
                        debug!("Allocated address {:#010x}", address);
                        root.set_bar_64(device_function, bar_index as u8, address.into());
                    }
                }

                _ => panic!("Memory BAR address type {:?} not supported.", address_type),
            }
        }
    }

    // Enable the device to use its BARs.
    root.set_command(
        device_function,
        Command::IO_SPACE | Command::MEMORY_SPACE | Command::BUS_MASTER,
    );
    let (status, command) = root.get_status_command(device_function);
    debug!(
        "Allocated BARs and enabled device, status {:?} command {:?}",
        status, command
    );
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    error!("{}", info);
    system_off::<Hvc>().unwrap();
    loop {}
}

const NET_QUEUE_SIZE: usize = 16;
type DeviceImpl<T> = VirtIONet<HalImpl, T, NET_QUEUE_SIZE>;

const IP: &str = "10.0.2.15"; // QEMU user networking default IP
const GATEWAY: &str = "10.0.2.2"; // QEMU user networking gateway
const PORT: u16 = 5555;

struct DeviceWrapper<T: Transport> {
    inner: Rc<RefCell<DeviceImpl<T>>>,
}

impl<T: Transport> DeviceWrapper<T> {
    fn new(dev: DeviceImpl<T>) -> Self {
        DeviceWrapper {
            inner: Rc::new(RefCell::new(dev)),
        }
    }

    fn mac_address(&self) -> EthernetAddress {
        EthernetAddress(self.inner.borrow().mac_address())
    }
}

impl<T: Transport> Device for DeviceWrapper<T> {
    type RxToken<'a>
        = VirtioRxToken<T>
    where
        Self: 'a;
    type TxToken<'a>
        = VirtioTxToken<T>
    where
        Self: 'a;

    fn receive(&mut self, _timestamp: Instant) -> Option<(Self::RxToken<'_>, Self::TxToken<'_>)> {
        match self.inner.borrow_mut().receive() {
            Ok(buf) => Some((
                VirtioRxToken(self.inner.clone(), buf),
                VirtioTxToken(self.inner.clone()),
            )),
            Err(Error::NotReady) => None,
            Err(err) => panic!("receive failed: {}", err),
        }
    }

    fn transmit(&mut self, _timestamp: Instant) -> Option<Self::TxToken<'_>> {
        Some(VirtioTxToken(self.inner.clone()))
    }

    fn capabilities(&self) -> DeviceCapabilities {
        let mut caps = DeviceCapabilities::default();
        caps.max_transmission_unit = 1536;
        caps.max_burst_size = Some(1);
        caps.medium = Medium::Ethernet;
        caps
    }
}

struct VirtioRxToken<T: Transport>(Rc<RefCell<DeviceImpl<T>>>, RxBuffer);
struct VirtioTxToken<T: Transport>(Rc<RefCell<DeviceImpl<T>>>);

impl<T: Transport> RxToken for VirtioRxToken<T> {
    fn consume<R, F>(self, f: F) -> R
    where
        F: FnOnce(&mut [u8]) -> R,
    {
        let mut rx_buf = self.1;
        info!(
            "RECV {} bytes: {:02X?}",
            rx_buf.packet_len(),
            rx_buf.packet()
        );
        let result = f(rx_buf.packet_mut());
        self.0.borrow_mut().recycle_rx_buffer(rx_buf).unwrap();
        result
    }
}

impl<T: Transport> TxToken for VirtioTxToken<T> {
    fn consume<R, F>(self, len: usize, f: F) -> R
    where
        F: FnOnce(&mut [u8]) -> R,
    {
        let mut dev = self.0.borrow_mut();
        let mut tx_buf = dev.new_tx_buffer(len);
        let result = f(tx_buf.packet_mut());
        info!("SEND {} bytes: {:02X?}", len, tx_buf.packet());
        dev.send(tx_buf).unwrap();
        result
    }
}

pub fn test_echo_server<T: Transport>(dev: DeviceImpl<T>) {
    let mut device = DeviceWrapper::new(dev);

    // Create interface
    let mut config = Config::new();
    config.random_seed = 0x2333;
    if device.capabilities().medium == Medium::Ethernet {
        config.hardware_addr = Some(device.mac_address().into());
    }

    let mut iface = Interface::new(config, &mut device);
    iface.update_ip_addrs(|ip_addrs| {
        ip_addrs
            .push(IpCidr::new(IpAddress::from_str(IP).unwrap(), 24))
            .unwrap();
    });

    iface
        .routes_mut()
        .add_default_ipv4_route(Ipv4Address::from_str(GATEWAY).unwrap())
        .unwrap();

    // Create sockets
    let tcp_rx_buffer = tcp::SocketBuffer::new(vec![0; 4096]);
    let tcp_tx_buffer = tcp::SocketBuffer::new(vec![0; 4096]);
    let tcp_socket = tcp::Socket::new(tcp_rx_buffer, tcp_tx_buffer);

    let mut sockets = SocketSet::new(vec![]);
    let tcp_handle = sockets.add(tcp_socket);

    info!("start a reverse echo server...");
    let mut tcp_active = false;
    loop {
        let mut tm = 0;
        tm += 1000;
        let timestamp = Instant::from_micros_const(tm);
        // let timestamp = Instant::from_micros_const(riscv::register::time::read() as i64 / 10);
        iface.poll(timestamp, &mut device, &mut sockets);

        // tcp:PORT: echo with reverse
        let socket = sockets.get_mut::<tcp::Socket>(tcp_handle);
        if !socket.is_open() {
            info!("listening on port {}...", PORT);
            socket.listen(PORT).unwrap();
        }

        if socket.is_active() && !tcp_active {
            info!("tcp:{} connected", PORT);
        } else if !socket.is_active() && tcp_active {
            info!("tcp:{} disconnected", PORT);
        }
        tcp_active = socket.is_active();

        if socket.may_recv() {
            let data = socket
                .recv(|buffer| {
                    let recvd_len = buffer.len();
                    if !buffer.is_empty() {
                        debug!("tcp:{} recv {} bytes: {:?}", PORT, recvd_len, buffer);
                        let mut lines = buffer
                            .split(|&b| b == b'\n')
                            .map(ToOwned::to_owned)
                            .collect::<Vec<_>>();
                        for line in lines.iter_mut() {
                            line.reverse();
                        }
                        let data = lines.join(&b'\n');
                        (recvd_len, data)
                    } else {
                        (0, vec![])
                    }
                })
                .unwrap();
            if socket.can_send() && !data.is_empty() {
                debug!("tcp:{} send data: {:?}", PORT, data);
                socket.send_slice(&data[..]).unwrap();
            }
        } else if socket.may_send() {
            info!("tcp:{} close", PORT);
            socket.close();
            break;
        }
    }
}

#[no_mangle]
extern "C" fn rs_log(log_level: u32, c_str: *const i8) {
    // Check if the c_str is not null
    if c_str.is_null() {
        // Handle null pointer, probably return or log an error
        return;
    }

    unsafe {
        // Find the length of the C string
        let mut len = 0;
        // Increment length until we encounter the null terminator
        while *c_str.add(len) != 0 {
            len += 1;
        }

        // Convert the C string to a byte slice
        let bytes = slice::from_raw_parts(c_str as *const u8, len);

        // Convert the byte slice to a Rust &str
        match str::from_utf8(bytes) {
            Ok(rust_str) => match log_level {
                0 => debug!("{}", rust_str),
                1 => info!("{}", rust_str),
                2 => error!("{}", rust_str),
                _ => error!("Invalid Log Level!"),
            },
            Err(_) => {
                error!("Invalid String!");
            }
        }
    }
}

///////// NET DRIVER

lazy_static! {
    static ref GLOBAL_TCP_HANDLE: spin::mutex::Mutex<Option<SocketHandle>> =
        spin::mutex::Mutex::new(None);
    static ref TCP_PCI_TRANSPORT: spin::mutex::Mutex<Option<PciTransport>> =
        spin::mutex::Mutex::new(None);
    static ref TCP_MMIO_TRANSPORT: spin::mutex::Mutex<Option<MmioTransport<'static>>> =
        spin::mutex::Mutex::new(None);
    static ref GLOBAL_TCP_IFACE: spin::mutex::Mutex<Option<Interface>> =
        spin::mutex::Mutex::new(None);
    static ref SOCKETS: spin::mutex::Mutex<SocketSet<'static>> =
        spin::mutex::Mutex::new(SocketSet::new(vec![]));
}

#[no_mangle]
pub fn setup_pci_tcp_handle() -> *mut DeviceWrapper<PciTransport> {
    const NET_BUFFER_LEN: usize = 4096 * 4;

    let mut pci_guard = TCP_PCI_TRANSPORT.lock();

    let mut pci_device: alloc::boxed::Box<DeviceWrapper<PciTransport>>;

    let mut pci_transport = pci_guard.take();

    if let Some(transport) = pci_transport {
        let net_result = virtio_drivers::device::net::VirtIONet::<
            HalImpl,
            PciTransport,
            NET_QUEUE_SIZE,
        >::new(transport, NET_BUFFER_LEN);

        if let Ok(net) = net_result {
            pci_device = alloc::boxed::Box::new(DeviceWrapper::new(net));
        } else {
            error!("Failed to initialize PCI device");
            return null_mut();
        }
    } else {
        error!("No Network Device!");
        return null_mut();
    }

    // Create interface
    let mut config = Config::new();
    config.random_seed = 0x2333;
    if pci_device.capabilities().medium == Medium::Ethernet {
        config.hardware_addr = Some(pci_device.mac_address().into());
    }

    let mut iface = Interface::new(config, pci_device.as_mut());
    iface.update_ip_addrs(|ip_addrs| {
        ip_addrs
            .push(IpCidr::new(IpAddress::from_str(IP).unwrap(), 24))
            .unwrap();
    });

    iface
        .routes_mut()
        .add_default_ipv4_route(Ipv4Address::from_str(GATEWAY).unwrap())
        .unwrap();

    // Create sockets
    let tcp_rx_buffer = tcp::SocketBuffer::new(vec![0; 4096]);
    let tcp_tx_buffer = tcp::SocketBuffer::new(vec![0; 4096]);
    let mut tcp_socket = tcp::Socket::new(tcp_rx_buffer, tcp_tx_buffer);
    tcp_socket.set_keep_alive(Some(Duration::from_millis(5000)));
    tcp_socket.set_timeout(Some(Duration::from_millis(500000)));

    let tcp_handle = SOCKETS.lock().add(tcp_socket);
    {
        GLOBAL_TCP_HANDLE.lock().get_or_insert(tcp_handle);
        GLOBAL_TCP_IFACE.lock().get_or_insert(iface);
    }

    let ptr = alloc::boxed::Box::into_raw(pci_device);
    info!("returning wrapper {:?}", ptr);
    return ptr;
}

#[no_mangle]
extern "C" fn network_is_setup() -> bool {
    return GLOBAL_TCP_HANDLE.lock().is_some();
}

#[no_mangle]
extern "C" fn network_receive_packet(
    pci_dev_ptr: *mut DeviceWrapper<PciTransport>,
    buffer: *mut u8,
    length: usize,
) -> usize {
    info!("Reading {} bytes, using wrapper {:?}", length, pci_dev_ptr);

    if pci_dev_ptr.is_null() {
        error!("No Network Device!");
        return 0;
    }

    let pci_device: &mut DeviceWrapper<PciTransport> = unsafe { &mut *pci_dev_ptr };

    // let mut sockets = SocketSet::new(vec![]);
    let tcp_handle = GLOBAL_TCP_HANDLE.lock().unwrap();
    let mut iface_guard = GLOBAL_TCP_IFACE.lock();
    let iface = iface_guard.as_mut().unwrap();
    let mut sockets = SOCKETS.lock();

    info!("start a reverse echo server...");
    let mut tcp_active = false;
    let mut bytes_recv = 0;

    let buff_slice = unsafe { slice::from_raw_parts_mut(buffer, length) };
    loop {
        let timestamp = Instant::from_micros_const(time_in_microseconds());

        iface.poll(timestamp, pci_device, &mut sockets);

        let socket = sockets.get_mut::<tcp::Socket>(tcp_handle);
        if !socket.is_open() {
            info!("listening on port {}...", PORT);
            socket.listen(PORT).unwrap();
        }

        if socket.is_active() && !tcp_active {
            info!("tcp:{} connected", PORT);
        } else if !socket.is_active() && tcp_active {
            info!("tcp:{} disconnected", PORT);
        }
        tcp_active = socket.is_active();

        if bytes_recv >= length {
            debug!("tcp:{} recv {} bytes: {:?}", PORT, length, buff_slice);
            decrypt(buffer, length);
            return length;
        }

        if socket.may_recv() {
            socket.recv(|buffer| {
                let recvd_len = buffer.len();
                let (max_len, cap) = {
                    if recvd_len + bytes_recv > length {
                        (length - bytes_recv, length)
                    } else {
                        (recvd_len, bytes_recv + recvd_len)
                    }
                };

                if !buffer.is_empty() {
                    debug!("tcp:{} recv {} bytes: {:?}", PORT, max_len, buffer);
                    (&mut buff_slice[bytes_recv..cap]).copy_from_slice(&buffer[..max_len]);
                }
                bytes_recv += max_len;
                (max_len, buffer)
            });
        }
    }
}

#[no_mangle]
extern "C" fn network_send_packet(
    pci_dev_ptr: *mut DeviceWrapper<PciTransport>,
    buffer: *mut u8,
    length: usize,
) -> usize {
    info!("Sending {} bytes, using wrapper {:?}", length, pci_dev_ptr);
    encrypt(buffer, length);

    if pci_dev_ptr.is_null() {
        error!("No Network Device!");
        return 0;
    }

    let pci_device: &mut DeviceWrapper<PciTransport> = unsafe { &mut *pci_dev_ptr };

    // let mut sockets = SocketSet::new(vec![]);
    let tcp_handle = GLOBAL_TCP_HANDLE.lock().unwrap();
    let mut iface_guard = GLOBAL_TCP_IFACE.lock();
    let iface = iface_guard.as_mut().unwrap();
    let mut sockets = SOCKETS.lock();

    let mut tcp_active = false;
    let mut bytes_sent = 0;

    let mut socket = sockets.get_mut::<tcp::Socket>(tcp_handle);
    loop {
        socket = sockets.get_mut::<tcp::Socket>(tcp_handle);
        if tcp_active && socket.can_send() {
            break;
        }

        if !socket.is_open() {
            info!("listening on port {}...", PORT);
            socket.listen(PORT).unwrap();
        }

        if socket.is_active() {
            info!("tcp:{} connected", PORT);
            tcp_active = socket.is_active();
        }

        let timestamp = Instant::from_micros_const(time_in_microseconds());
        iface.poll(timestamp, pci_device, &mut sockets);
    }

    if socket.can_send() && !buffer.is_null() {
        let buff_slice = unsafe { slice::from_raw_parts(buffer, length) };
        bytes_sent = match socket.send_slice(&buff_slice[..]) {
            Ok(len) => len,
            Err(_) => 0,
        };
        info!("Sent {} bytes: {:?}", length, buff_slice);
    }

    loop {
        let timestamp = Instant::from_micros_const(time_in_microseconds());
        iface.poll(timestamp, pci_device, &mut sockets);

        socket = sockets.get_mut::<tcp::Socket>(tcp_handle);
        if !socket.is_open() {
            info!("listening on port {}...", PORT);
            socket.listen(PORT).unwrap();
        }
        if socket.send_queue() == 0 {
            return bytes_sent;
        }

        if socket.is_active() && !tcp_active {
            info!("tcp:{} connected", PORT);
        } else if !socket.is_active() && tcp_active {
            info!("tcp:{} disconnected", PORT);
            return bytes_sent;
        }
        tcp_active = socket.is_active();
    }
}

extern crate rand_core;

use aes::cipher::KeyInit;
use aes::{
    cipher::{KeyIvInit, StreamCipher},
    Aes256,
};
use ctr::Ctr128BE; // Big-Endian (BE) counter, there is also Ctr128LE for Little-Endian
use ctr::Ctr128LE; // Big-Endian (BE) counter, there is also Ctr128LE for Little-Endian
use generic_array::{ArrayLength, GenericArray};
use hex_literal::hex; // Import traits for initialization
                      //

type Aes256Ctr = Ctr128BE<Aes256>;

#[no_mangle]
extern "C" fn decrypt(msg: *mut u8, len: usize) {
    let key = hex!("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    let iv = hex!("00112233445566778899aabbccddeeff"); // 16-byte nonce for AES CTR
    let mut buffer = unsafe { slice::from_raw_parts_mut(msg, len) };

    let mut decryptor = Aes256Ctr::new(&key.into(), &iv.into());

    // Decrypt the message in-place
    decryptor.apply_keystream(buffer);

    // Print or inspect decrypted text
    info!("Decrypted text: {:?}", buffer);
}

#[no_mangle]
extern "C" fn encrypt(msg: *mut u8, len: usize) {
    let key = hex!("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    let iv = hex!("00112233445566778899aabbccddeeff"); // 16-byte nonce for AES CTR
    let mut buffer = unsafe { slice::from_raw_parts_mut(msg, len) };

    // Create an AES-256 CTR mode cipher instance with key and nonce
    let mut cipher = Aes256Ctr::new(&key.into(), &iv.into());

    // Encrypt the message in-place
    cipher.apply_keystream(buffer);
    // Print or inspect decrypted text
    info!("Encrypted text: {:?}", buffer);
}

#[no_mangle]
pub extern "C" fn destroy_pci_device(pci_dev_ptr: *mut DeviceWrapper<PciTransport>) {
    if pci_dev_ptr.is_null() {
        return;
    }
    // Transform the raw pointer back into a Box, which will be dropped immediately
    unsafe {
        alloc::boxed::Box::from_raw(pci_dev_ptr);
    }
}
