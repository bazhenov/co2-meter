use std::env;
use std::io;
use std::time::Duration;

use serial::prelude::*;
use std::convert::TryInto;

use prometheus_exporter::{
    self,
    prometheus::register_int_gauge_vec,
};

fn main() {
    for arg in env::args_os().skip(1) {
        let mut port = serial::open(&arg).unwrap();
        interact(&mut port).unwrap();
    }
}

fn interact<T: SerialPort>(port: &mut T) -> io::Result<()> {

    let binding = "0.0.0.0:9185".parse().unwrap();
    let exporter = prometheus_exporter::start(binding).unwrap();

    let gauge = register_int_gauge_vec!("ADC", "ADC channel value", &["channel"]).unwrap();

    let settings = serial::PortSettings {
        baud_rate: serial::Baud9600,
        char_size: serial::Bits8,
        parity: serial::ParityNone,
        stop_bits: serial::Stop1,
        flow_control: serial::FlowNone
    };
    port.configure(&settings)?;

    port.set_timeout(Duration::from_millis(5000))?;

    let mut buf: Vec<u8> = vec![0; 4];

    while port.read_exact(&mut buf[..]).is_ok() {
        let value = u16::from_le_bytes(buf[2..4].try_into().unwrap());
        let voltage: f32 = (f32::from(value) / 1024f32) * 5f32;
        let co2 = (((voltage - 1f32) / 4f32) * 2000f32) as i64;
        println!("({}), {}", value, co2);
        let _guard = exporter.wait_duration(Duration::from_millis(100));
        gauge.with_label_values(&["1"]).set(co2);
    }

    Ok(())
}