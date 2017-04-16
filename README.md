# TinyFrame

TinyFrame is a simple library for building and parsing frames to be sent 
over a serial interface (e.g. UART, telnet etc.). The code is written 
in `--std=gnu89`.

Frames are protected by a checksum (~XOR, CRC16 or CRC32) and contain 
a unique ID field, which can be used for chaining messages. The highest value 
of the ID is different for each peer (TF_MASTER or TF_SLAVE) to avoid collisions.

All fields in the frame have configurable size (see the top of the header file).
By just changing a value, like `TF_LEN_BYTES`, the library seamlessly switches 
from `uint8_t` to `uint16_t` or `uint32_t` to support longer payloads.

The library lets you bind listeners waiting for any frame, a particular frame Type,
or a specific message ID. This lets you easily implement asynchronous
communication.

## Frame structure

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

- Both peers must include the library with the same parameters (config in the header file)
- Start by calling `TF_Init()` with MASTER or SLAVE as the argument
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
volatile bool onResponse_done = false;

/** ID listener */
bool onResponse(TF_ID frame_id, TF_TYPE type, const uint8_t *data, TF_LEN len)
{
    // ... Do something ... 
    // (eg. copy data to a global variable)
    
    onResponse_done = true;
    return true;
}

bool syncQuery(void) 
{
    TF_ID id;
    // Send our request
    TF_Send0(MSG_PING, onResponse, &id); // Send0 sends zero bytes of data, just TYPE
    
    // Wait for the response
    bool suc = true;
    while (!onResponse_done) {
        //delay
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
