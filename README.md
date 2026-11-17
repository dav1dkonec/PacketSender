# udp_sender

Paced generator of `LAT1` test packets for [latency_rx](https://github.com/dav1dkonec/PacketReceiver) (macOS and Linux, C++20).

`include/packet.h` is a copy of the receiver's packet format, keep both identical.

## Build

```sh
cmake -B build && cmake --build build
ctest --test-dir build
```

