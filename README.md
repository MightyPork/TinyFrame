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


## Usage hints

- Both sides of the protocol (slave and master) should include the same TinyFrame
code.
- Master inits the lib with `TF_Init(1);`, while slave uses `TF_Init(0);`. This is to avoid a message ID conflict.
- Both sides can add Generic and Type listeners (callbacks) using `TF_AddGenericListener(func)` and `TF_AddTypeListener(type, func)`. The listener is a function as showin in the example file test.c or declared in TinyFrame.h

  `bool myListener(unsigned int frame_id, const unsigned char *buff, unsigned int len) { ... }`

  The listener returns `true` if the message was consumed. If it returns `false`, it can be handled by some other listener (possibly a Generic Listener, if you added one)
- A message is sent using `TF_Send()`, and to use it, the `TF_WriteImpl()` stub must be implemented in the application code. See `test.c` for an example.

  There are also helper functions `TF_Send1()` and `TF_Send2()` which send one or two bytes.
- To reply, use `TF_Respond()` with ID same as in the received message (the listener gets this as it's argument). A listener provided as the last parameter to `TF_Send()` will be called after receiving the response.

- To remove a listener, use `TF_RemoveListener()`. *Always remove your ID listeners after handling the response!* There's a limit to the number of listeners.

- The function `TF_Accept()` is used to handle received chars. Call this in your UART Rx interrupt handler or a similar place.
