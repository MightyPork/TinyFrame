# TinyFrame

TinyFrame is a simple library for building and parsing frames to be sent 
over a serial interface (e.g. UART, telnet etc.). The code is written 
to build with `--std=gnu89` and later.

TinyFrame is suitable for a wide range of applications, including inter-microcontroller 
communication, as a protocol for FTDI-based PC applications or for messaging through
UDP packets. If you find a good use for it, please let me know so I can add it here!

Frames can be protected by a checksum (~XOR, CRC16 or CRC32) and contain 
a unique ID field which can be used for chaining related messages. The highest bit 
of the generated IDs is different in each peer to avoid collisions.
Peers are functionally equivalent and can send messages to each other 
(the names "master" and "slave" are used only for convenience and have special meaning 
in the demos).

The library lets you register listeners (callback functions) to wait for (1) any frame, (2)
a particular frame Type, or (3) a specific message ID. This  high-level API lets you 
easily implement various async communication patterns.

TinyFrame is re-entrant and supports creating multiple instances with the limitation
that their structure (field sizes and checksum type) must be the same. There is a support
for adding multi-threaded access to a shared instance using a mutex (via a callback stub).

## Frame structure

All fields in the message frame have a configurable size (see the top of the header file).
By just changing a definition in the header, such as `TF_LEN_BYTES` (1, 2 or 4), the library
seamlessly switches between `uint8_t`, `uint16_t` and `uint32_t`. Choose field lengths that
best suit your application needs. 

For example, you don't need 4 bytes (`uint32_t`) for the 
length field if your payloads are 20 bytes long, using a 1-byte field (`uint8_t`) will save
3 bytes. This may be significant if you high throughput.

```
,-----+----+-----+------+------------+- - - -+------------,                
| SOF | ID | LEN | TYPE | HEAD_CKSUM | DATA  | PLD_CKSUM  |                
| 1   | ?  | ?   | ?    | ?          | ...   | ?          | <- size (bytes)
'-----+----+-----+------+------------+- - - -+------------'                

SOF ......... start of frame, 0x01
ID  ......... the frame ID (MSb is the peer bit)
LEN ......... nr of data bytes in the frame
TYPE ........ message type (used to run Type Listeners, pick any values you like)
HEAD_CKSUM .. header checksum
DATA ........ LEN bytes of data
DATA_CKSUM .. checksum, implemented as XOR of all preceding bytes in the message
```

## Usage Hints

- All TinyFrame functions, typedefs and macros start with the `TF_` prefix.
- Both peers must include the library with the same parameters (configured at the top of the header file)
- Start by calling `TF_Init()` with `TF_MASTER` or `TF_SLAVE` as the argument. This creates a handle.
  Use `TF_InitStatic()` to avoid the use of malloc(). If multiple instances are used, you can tag them 
  using the `tf.userdata` / `tf.usertag` field.
- Implement `TF_WriteImpl()` - declared at the bottom of the header file as `extern`.
  This function is used by `TF_Send()` and others to write bytes to your UART (or other physical layer).
  A frame can be sent in it's entirety, or in multiple parts, depending on its size.
- If you wish to use timeouts, periodically call `TF_Tick()`. The calling period determines 
  the length of 1 tick. This is used to time-out the parser in case it gets stuck 
  in a bad state (such as receiving a partial frame) and can also time-out ID listeners.
- Bind Type or Generic listeners using `TF_AddTypeListener()` or `TF_AddGenericListener()`.
- Send a message using `TF_Send()`, `TF_Query()`, `TF_SendSimple()`, `TF_QuerySimple()`.
  Query functions take a listener callback (function pointer)that will be added as 
  an ID listener and wait for a response.
- To reply to a message (when your listener gets called), use `TF_Respond()`
  with the msg boject you received, replacing the `data` pointer (and `len`) with response.
- Manually reset the parser using `TF_ResetParser()`

### Message listeners

Listeners are callback functions that are called by TinyFrame when a message which 
they can handle is received.

There are 3 listener types:
 
- ID listeners
- Type listeners
- Generic listeners

Listeners return an enum constant based on what should be done next - remove the listener, 
keep it, renew it's timeout, or let some other listener handle the message.

### Examples

You'll find various examples in the `demo/` folder. Each example has it's own Makefile,
read it to see what options are available.

The demos are written for Linux, some using sockets and `clone()` for background processing.
They try to simulate real TinyFrame behavior in an embedded system with asynchronous 
Rx and Tx. If you can't run the demos, the source files are still good as examples.
