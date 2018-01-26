# TinyFrame

TinyFrame is a simple library for building and parsing data frames to be sent 
over a serial interface (e.g. UART, telnet, socket). The code is written to build with 
`--std=gnu99` and mostly compatible with `--std=gnu89`.

The library provides a high level interface for passing messages between the two peers.
Multi-message sessions, response listeners, checksums, timeouts are all handled by the library.

TinyFrame is suitable for a wide range of applications, including inter-microcontroller 
communication, as a protocol for FTDI-based PC applications or for messaging through
UDP packets.

The library lets you register listeners (callback functions) to wait for (1) any frame, (2)
a particular frame Type, or (3) a specific message ID. This high-level API is general 
enough to implement most communication patterns.

TinyFrame is re-entrant and supports creating multiple instances with the limitation
that their structure (field sizes and checksum type) is the same. There is a support
for adding multi-threaded access to a shared instance using a mutex.

TinyFrame also comes with (optional) helper functions for building and parsing message
payloads, those are provided in the `utils/` folder.

## Ports

TinyFrame has been ported to mutiple languages:

- The reference C implementation is in this repo
- Python port - [MightyPork/PonyFrame](https://github.com/MightyPork/PonyFrame)
- Rust port - [cpsdqs/tinyframe-rs](https://github.com/cpsdqs/tinyframe-rs)
- JavaScript port - [cpsdqs/tinyframe-js](https://github.com/cpsdqs/tinyframe-js)

Please note most of the ports are experimental and may exhibit various bugs or missing 
features. Testers are welcome :)

## Functional overview

The basic functionality of TinyFrame is explained here. For particlars, such as the
API functions, it's recommended to read the doc comments in the header file.

### Structure of a frame

Each frame consists of a header and a payload. Both parts can be protected by a checksum, 
ensuring a frame with a malformed header (e.g. with a corrupted length field) or a corrupted
payload is rejected.

The frame header contains a frame ID and a message type. Frame ID is incremented with each
new message. The highest bit of the ID field is fixed to 1 and 0 for the two peers, 
avoiding a conflict.

Frame ID can be re-used in a response to tie the two messages together. Values of the
type field are user defined.

All fields in the frame have a configurable size. By changing a field in the config 
file, such as `TF_LEN_BYTES` (1, 2 or 4), the library seamlessly switches between `uint8_t`,
`uint16_t` and `uint32_t` for all functions working with the field. 

```
,-----+-----+-----+------+------------+- - - -+-------------,
| SOF | ID  | LEN | TYPE | HEAD_CKSUM | DATA  | DATA_CKSUM  |
| 0-1 | 1-4 | 1-4 | 1-4  | 0-4        | ...   | 0-4         | <- size (bytes)
'-----+-----+-----+------+------------+- - - -+-------------'

SOF ......... start of frame, usually 0x01 (optional, configurable)
ID  ......... the frame ID (MSb is the peer bit)
LEN ......... number of data bytes in the frame
TYPE ........ message type (used to run Type Listeners, pick any values you like)
HEAD_CKSUM .. header checksum

DATA ........ LEN bytes of data
DATA_CKSUM .. data checksum (left out if LEN is 0)
```

### Message listeners

TinyFrame is based on the concept of message listeners. A listener is a callback function 
waiting for a particular message Type or ID to be received.

There are 3 listener types, in the order of precedence:
 
- **ID listeners** - waiting for a response
- **Type listeners** - waiting for a message of the given Type field
- **Generic listeners** - fallback

ID listeners can be registered automatically when sending a message. All listeners can 
also be registered and removed manually. 

ID listeners are used to receive the response to a request. When registerign an ID 
listener, it's possible to attach custom user data to it that will be made available to 
the listener callback. This data (`void *`) can be any kind of application context 
variable.

ID listeners can be assigned a timeout. When a listener expires, before it's removed,
the callback is fired with NULL payload data in order to let the user `free()` any
attached userdata. This happens only if the userdata is not NULL.

Listener callbacks return values of the `TF_Result` enum:

- `TF_CLOSE` - message accepted, remove the listener
- `TF_STAY` - message accepted, stay registered
- `TF_RENEW` - sameas `TF_STAY`, but the ID listener's timeout is renewed
- `TF_NEXT` - message NOT accepted, keep the listener and pass the message to the next 
              listener capable of handling it.

### Data buffers, multi-part frames

TinyFrame uses two data buffers: a small transmit buffer and a larger receive buffer.
The transmit buffer is used to prepare bytes to send, either all at once, or in a 
circular fashion if the buffer is not large enough. The buffer must only contain the entire 
frame header, so e.g. 32 bytes should be sufficient for short messages.

Using the `*_Multipart()` sending functions, it's further possible to split the frame 
header and payload to multiple function calls, allowing the applciation to e.g. generate
the payload on-the-fly.

In contrast to the transmit buffer, the receive buffer must be large enough to contain 
an entire frame. This is because the final checksum must be verified before the frame 
is handled.
 
If frames larger than the possible receive buffer size are required (e.g. in embedded 
systems with small RAM), it's recommended to implement a multi-message transport mechanism
at a higher level and send the data in chunks.

## Usage Hints

- All TinyFrame functions, typedefs and macros start with the `TF_` prefix.
- Both peers must include the library with the same config parameters
- See `TF_Integration.example.c` and `TF_Config.example.c` for reference how to configure and integrate the library.
- DO NOT modify the library files, if possible. This makes it easy to upgrade.
- Start by calling `TF_Init()` with `TF_MASTER` or `TF_SLAVE` as the argument. This creates a handle.
  Use `TF_InitStatic()` to avoid the use of malloc(). 
- If multiple instances are used, you can tag them using the `tf.userdata` / `tf.usertag` field.
- Implement `TF_WriteImpl()` - declared at the bottom of the header file as `extern`.
  This function is used by `TF_Send()` and others to write bytes to your UART (or other physical layer).
  A frame can be sent in it's entirety, or in multiple parts, depending on its size.
- If you wish to use timeouts, periodically call `TF_Tick()`. The calling period determines 
  the length of 1 tick. This is used to time-out the parser in case it gets stuck 
  in a bad state (such as receiving a partial frame) and can also time-out ID listeners.
- Bind Type or Generic listeners using `TF_AddTypeListener()` or `TF_AddGenericListener()`.
- Send a message using `TF_Send()`, `TF_Query()`, `TF_SendSimple()`, `TF_QuerySimple()`.
  Query functions take a listener callback (function pointer) that will be added as 
  an ID listener and wait for a response.
- Use the `*_Multipart()` variant of the above sending functions for payloads generated in
  multiple function calls. The payload is sent afterwards by calling `TF_Multipart_Payload()`
  and the frame is closed by `TF_Multipart_Close()`.
- If custom checksum implementation is needed, select `TF_CKSUM_CUSTOM8`, 16 or 32 and 
  implement the three checksum functions.
- To reply to a message (when your listener gets called), use `TF_Respond()`
  with the msg object you received, replacing the `data` pointer (and `len`) with a response.
- At any time you can manually reset the message parser using `TF_ResetParser()`. It can also 
  be reset automatically after a timeout configured in the config file.

### Gotchas to look out for

- If any userdata is attached to an ID listener with a timeout, when the listener times out,
  it will be called with NULL `msg->data` to let the user free the userdata. Therefore 
  it's needed to check `msg->data` before proceeding to handle the message.
- If a multi-part frame is being sent, the Tx part of the library is locked to prevent 
  concurrent access. The frame must be fully sent and closed before attempting to send
  anything else. 
- If multiple threads are used, don't forget to implement the mutex callbacks to avoid 
  concurrent access to the Tx functions. The default implementation is not entirely thread
  safe, as it can't rely on platform-specific resources like mutexes or atomic access. 
  Set `TF_USE_MUTEX` to `1` in the config file.

### Examples

You'll find various examples in the `demo/` folder. Each example has it's own Makefile,
read it to see what options are available.

The demos are written for Linux, some using sockets and `clone()` for background processing.
They try to simulate real TinyFrame behavior in an embedded system with asynchronous 
Rx and Tx. If you can't run the demos, the source files are still good as examples.
