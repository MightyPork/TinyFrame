//---------------------------------------------------------------------------
#include "TinyFrame.h"
#include <string.h>
//---------------------------------------------------------------------------

/* Note: payload length determines the Rx buffer size. Max 256 */
#define TF_MAX_PAYLOAD 256
#define TF_SOF_BYTE 0x01


enum TFState {
    TFState_SOF = 0,  //!< Wait for SOF
    TFState_ID,       //!< Wait for ID
    TFState_NOB,      //!< Wait for Number Of Bytes
    TFState_PAYLOAD,  //!< Receive payload
    TFState_CKSUM     //!< Wait for Checksum
};

typedef struct _IdListener_struct {
    unsigned int id;
    TinyFrameListener fn;
} IdListener;

typedef struct _TypeListener_struct_ {
    unsigned char type;
    TinyFrameListener fn;
} TypeListener;

typedef struct _GenericListener_struct_ {
    TinyFrameListener fn;
} GenericListener;

/**
 * Frame parser internal state
 */
static struct TinyFrameStruct {
    /* Own state */
    bool peer_bit;          //!< Own peer bit (unqiue to avoid msg ID clash)
    unsigned int next_id;   //!< Next frame / frame chain ID

    /* Parser state */
    enum TFState state;
    unsigned int id;        //!< Incoming packet ID
    unsigned int nob;       //!< Payload length
    unsigned char pldbuf[TF_MAX_PAYLOAD+1]; //!< Payload byte buffer
    unsigned int rxi;       //!< Receive counter (for payload or other fields)
    unsigned int cksum;     //!< Continually updated checksum

    /* --- Callbacks --- */

    /* Transaction callbacks */
    IdListener id_listeners[TF_MAX_ID_LST];
    TypeListener type_listeners[TF_MAX_TYPE_LST];
    GenericListener generic_listeners[TF_MAX_GEN_LST];

    char sendbuf[TF_MAX_PAYLOAD+1];
} tf;

void TF_Init(bool peer_bit)
{
    memset(&tf, 0, sizeof(struct TinyFrameStruct));

    tf.peer_bit = peer_bit;
}

void TF_ResetParser(void)
{
    tf.state = TFState_SOF;
    tf.cksum = 0;
}

int TF_AddIdListener(unsigned int frame_id, TinyFrameListener cb)
{
    int i;

    for (i = 0; i < TF_MAX_ID_LST; i++) {
        if (tf.id_listeners[i].fn == NULL) {
            tf.id_listeners[i].fn = cb;
            tf.id_listeners[i].id = frame_id;
            return i;
        }
    }

    return TF_ERROR;
}

int TF_AddTypeListener(unsigned char frame_type, TinyFrameListener cb)
{
    int i;

    for (i = 0; i < TF_MAX_TYPE_LST; i++) {
        if (tf.type_listeners[i].fn == NULL) {
            tf.type_listeners[i].fn = cb;
            tf.type_listeners[i].type = frame_type;
            return TF_MAX_ID_LST + i;
        }
    }

    return TF_ERROR;
}

int TF_AddGenericListener(TinyFrameListener cb)
{
    int i;

    for (i = 0; i < TF_MAX_GEN_LST; i++) {
        if (tf.generic_listeners[i].fn == NULL) {
            tf.generic_listeners[i].fn = cb;
            return TF_MAX_ID_LST + TF_MAX_TYPE_LST + i;
        }
    }

    return TF_ERROR;
}

void TF_RemoveListener(unsigned int index)
{
    // all listener arrays share common "address space"
    if (index < TF_MAX_ID_LST) {
        tf.id_listeners[index].fn = NULL;
    }
    else if (index < TF_MAX_ID_LST + TF_MAX_TYPE_LST) {
        tf.type_listeners[index - TF_MAX_ID_LST].fn = NULL;
    }
    else if (index < TF_MAX_ID_LST + TF_MAX_TYPE_LST + TF_MAX_GEN_LST) {
        tf.generic_listeners[index - TF_MAX_ID_LST - TF_MAX_TYPE_LST].fn = NULL;
    }
}

void TF_RemoveIdListener(unsigned int frame_id)
{
    int i;
    for (i = 0; i < TF_MAX_ID_LST; i++) {
        if (tf.id_listeners[i].fn != NULL && tf.id_listeners[i].id == frame_id) {
            tf.id_listeners[i].fn = NULL;
        }
    }
}

void TF_RemoveTypeListener(unsigned char type)
{
    int i;
    for (i = 0; i < TF_MAX_TYPE_LST; i++) {
        if (tf.type_listeners[i].fn != NULL && tf.type_listeners[i].type == type) {
            tf.type_listeners[i].fn = NULL;
        }
    }
}

void TF_RemoveListenerFn(TinyFrameListener cb)
{
    int i;

    for (i = 0; i < TF_MAX_ID_LST; i++) {
        if (tf.id_listeners[i].fn == cb) {
            tf.id_listeners[i].fn = NULL;
        }
    }

    for (i = 0; i < TF_MAX_TYPE_LST; i++) {
        if (tf.type_listeners[i].fn == cb) {
            tf.type_listeners[i].fn = NULL;
        }
    }

    for (i = 0; i < TF_MAX_GEN_LST; i++) {
        if (tf.generic_listeners[i].fn == cb) {
            tf.generic_listeners[i].fn = NULL;
        }
    }
}

void TF_Accept(const unsigned char *buffer, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        TF_AcceptChar(buffer[i]);
    }
}

void TF_AcceptChar(unsigned char c)
{
    int i;
    bool rv, brk;

    switch (tf.state)
    {
    case TFState_SOF:
        if (c == TF_SOF_BYTE) {
            tf.cksum = 0;
            tf.state = TFState_ID;
        }
        break;

    case TFState_ID:
        tf.id = c;
        tf.state = TFState_NOB;
        break;

    case TFState_NOB:
        tf.nob = c + 1; // using 0..255 as 1..256
        tf.state = TFState_PAYLOAD;
        tf.rxi = 0;
        break;

    case TFState_PAYLOAD:
        tf.pldbuf[tf.rxi++] = c;
        if (tf.rxi == tf.nob) {
            tf.state = TFState_CKSUM;
        }
        break;

    case TFState_CKSUM:
        tf.state = TFState_SOF;

        if (tf.cksum == (unsigned int)c) {
            // Add 0 at the end of the data in the buffer (useful if it was a string)
            tf.pldbuf[tf.rxi] = '\0';
            brk = false;

            // Fire listeners
            for (i = 0; i < TF_MAX_ID_LST; i++) {
                if (tf.id_listeners[i].fn && tf.id_listeners[i].id == tf.id) {
                    rv = tf.id_listeners[i].fn(tf.id, tf.pldbuf, tf.nob);
                    if (rv) {
                        brk = true;
                        break;
                    }
                }
            }

            if (!brk) {
                for (i = 0; i < TF_MAX_TYPE_LST; i++) {
                    if (tf.type_listeners[i].fn &&
                        tf.type_listeners[i].type == tf.pldbuf[0]) {
                        rv = tf.type_listeners[i].fn(tf.id, tf.pldbuf, tf.nob);
                        if (rv) {
                            brk = true;
                            break;
                        }
                    }
                }
            }

            if (!brk) {
                for (i = 0; i < TF_MAX_GEN_LST; i++) {
                    if (tf.generic_listeners[i].fn) {
                        rv = tf.generic_listeners[i].fn(tf.id, tf.pldbuf, tf.nob);
                        if (rv) {
                            break;
                        }
                    }
                }
            }
        } else {
            // Fail, return to base state
            tf.state = TFState_SOF;
        }
        break;
    }

    // Update the checksum
    tf.cksum ^= c;
}

int TF_Compose(unsigned char *outbuff, unsigned int *msgid,
               const unsigned char *payload, unsigned int payload_len,
               int explicit_id
) {
    unsigned int i;
    unsigned int id;
    int xor;

    // sanitize len
    if (payload_len > TF_MAX_PAYLOAD) return TF_ERROR;
    if (payload_len == 0) payload_len = strlen(payload);

    // Gen ID
    if (explicit_id == TF_NEXT_ID) {
        id = tf.next_id++;
        if (tf.peer_bit) {
            id |= 0x80;
        }

        if (tf.next_id > 0x7F) {
            tf.next_id = 0;
        }
    } else {
        id = explicit_id;
    }

    outbuff[0] = TF_SOF_BYTE;
    outbuff[1] = id & 0xFF;
    outbuff[2] = (payload_len - 1) & 0xFF; // use 0..255 as 1..256
    memcpy(outbuff+3, payload, payload_len);

    xor = 0;
    for (i = 0; i < payload_len + 3; i++) {
        xor ^= outbuff[i];
    }

    outbuff[payload_len + 3] = xor;

    if (msgid != NULL) *msgid = id;

    return payload_len + 4;
}

int TF_Send(const unsigned char *payload,
             unsigned int payload_len,
             TinyFrameListener listener,
             unsigned int *id_ptr)
{
    unsigned int msgid;
    int len;
    int lstid = TF_ERROR;
    len = TF_Compose(tf.sendbuf, &msgid, payload, payload_len, TF_NEXT_ID);
    if (listener) lstid = TF_AddIdListener(msgid, listener);
    TF_WriteImpl(tf.sendbuf, len);

    if (id_ptr) *id_ptr = msgid;

    return lstid;
}

void TF_Respond(const unsigned char *payload,
               unsigned int payload_len,
               unsigned int frame_id)
{
    int len;
    len = TF_Compose(tf.sendbuf, NULL, payload, payload_len, frame_id);
    TF_WriteImpl(tf.sendbuf, len);
}


int TF_Send1(const unsigned char b0,
             TinyFrameListener listener,
             unsigned int *id_ptr)
{
    unsigned char b[] = {b0};
    return TF_Send(b, 1, listener, id_ptr);
}


int TF_Send2(const unsigned char b0,
             const unsigned char b1,
             TinyFrameListener listener,
             unsigned int *id_ptr)
{
    unsigned char b[] = {b0, b1};
    return TF_Send(b, 2, listener, id_ptr);
}
