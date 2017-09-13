# TinyFrame

TinyFrame is a simple library for building and parsing frames to be sent 
over a serial interface (e.g. UART, telnet etc.). The code is written 
to build with `--std=gnu89` and later.

TinyFrame is suitable for a wide range of applications, including inter-microcontroller 
communication, as a protocol for FTDI-based PC applications or for messaging through
UDP packets. If you find a good use for it, please let me know so I can add it here!

Frames can be protected by a checksum (~XOR, CRC16 or CRC32) and contain 
a unique ID field which can be used for chaining related messages. The highest bit 
of the generated IDs is different for each peer to avoid collisions.
Peers are functionally equivalent and can send messages to each other 
(the names "master" and "slave" are used only for convenience).

The library lets you register listeners (callback functions) to wait for (1) any frame, (2)
a particular frame Type, or (3) a specific message ID. This lets you easily implement asynchronous
communication.

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
- Start by calling `TF_Init()` with `TF_MASTER` or `TF_SLAVE` as the argument
- Implement `TF_WriteImpl()` - declared at the bottom of the header file as `extern`.
  This function is used by `TF_Send()` to write bytes to your UART (or other physical layer).
  Presently, always a full frame is sent to this function.
- If you wish to use `TF_PARSER_TIMEOUT_TICKS`, periodically call `TF_Tick()`. The period 
  determines the length of 1 tick. This is used to time-out the parser in case it gets stuck 
  in a bad state (such as receiving a partial frame).
- Bind Type or Generic listeners using `TF_AddTypeListener()` or `TF_AddGenericListener()`.
- Send a message using `TF_Send()` or the other Send functions.
  If you provide a listener callback (function pointer) to the function,
  the listener will be added as an ID listener and wait for a response.
- To reply to a message (when your listener gets called), use `TF_Respond()`
  with the same frame_id as in the received message.
- Remove the ID listener using `TF_RemoveIdListener()` when it's no longer
  needed. (Same for other listener types.) The slot count is limited.
- If the listener function returns `false`, some other listener will get
  a chance to handle it
- Manually reset the parser using `TF_ResetParser()`

### The concept of listeners

Listeners are callback functions that are called by TinyFrame when a message which 
they can handle is received.

There are 3 listener types:
 
- ID listeners
- Type listeners
- Generic listeners

They handle the message in this order, and if they decide not to handle it, they can return `false`
and let it be handled by some other listener, or discarded.

### Implementing "synchronous query"

Sometimes it's necessary to send a message and wait for a response to arrive.

One (not too pretty) way to do this is using a global variable - pseudocode:

```c
#define MSG_PING 42

static volatile bool got_response = false;

/** ID listener */
static bool onResponse(TF_ID frame_id, TF_TYPE type, const uint8_t *data, TF_LEN len)
{
    // ... Do something ... 
    // (eg. copy data to a global variable)
    
    got_response = true;
    return true;
}

bool syncQuery(void) 
{
    TF_ID id;
    // Send our request, and bind an ID listener
    got_response = false;
    TF_Send0(MSG_PING, onResponse, &id); // Send0 sends zero bytes of data, just TYPE
    
    // the ID is now in `id` so we can remove the listener after a timeout
    
    // Wait for the response
    bool suc = true;
    while (!got_response) {
        //delay()
        if (/*timeout*/) {
            TF_RemoveIdListener(id); // free the listener slot
            return false;
        }
    }
    
    // ... Do something with the received data? ...
    // (can be passed from the listener using a global variable)
    
    return true;
}
```
