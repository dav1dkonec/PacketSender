# udp_sender

Paced generator of `LAT1` test packets for [latency_rx](https://github.com/dav1dkonec/PacketReceiver) (macOS and Linux, C++20).

`include/packet.h` is a copy of the receiver's packet format, keep both identical.

## Build

```sh
cmake -B build && cmake --build build
ctest --test-dir build
```

## Usage

```sh
./build/udp_sender -d 10.0.0.2 -n 10000 -i 1000 -w 20 -R
./build/udp_sender -d 10.0.0.2 -L      # receiver in packet mode (no UDP listener)
```

Without `-L` the sender stops when the receiver is unreachable
(ICMP port/host unreachable, no route).

`-R` (macOS: time constraint policy, Linux: SCHED_FIFO 80 + mlockall) keeps the
send times within a few us of the schedule; without it macOS wakes the thread
up to ~1-4 ms late. On Linux `-R` needs CAP_SYS_NICE and CAP_IPC_LOCK (or root),
`-I` needs CAP_NET_RAW only on kernels older than 5.7.
An absent host is detected only after the ARP timeout (~3.5 s).
