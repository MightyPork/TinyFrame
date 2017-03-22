# TinyFrame

TinyFrame is a simple library for building and parsing frames
(packets) to be sent over a serial interface (like UART). It's implemented
to be compatible with C89 and platform agnostic.

Frames are protected by a checksum and contain a "unique" ID,
which can be used for chaining messages. Each peer uses a different
value for the first bit of all IDs it generates (the "master flag"
or "peer_bit") to ensure there are no clashes. Typically the master
(PC, main microcontroller) will use "1" and the surrogate (WiFi module,
USB-serial connected gadget, display driver...) uses "0".

The library lets you bind listeners waiting for any frame, or a
particular ID. This allows for easy implementation of async communication.

## Frame structure

The frame makeup is inspired by that of SBMP (my other, more complicated
and advanced UART protocol library).

```
<SOF><ID><NOB><PAYLOAD><CKSUM>

SOF ... start of frame, 0x01
ID  ... (master_flag | 7-bit counter) - the frame ID
NOB ... nr of payload bytes in the frame (1..256)
PAYLOAD ... NOB bytes of data, can contain any byte values 1..256
CKSUM ... checksum, implemented as XOR of all preceding bytes in the message
```

The frame ID (in SBMP called "session ID") can be used to chain multiple related
messages and maintain the context this way. For example, a response may copy
the frame ID of the request frame, which then triggers a callback bound by the
requesting peer. Such behavior is application specific and is thus left to the
upper layers of the protocol.
