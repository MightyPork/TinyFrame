#ifndef TinyFrameH
#define TinyFrameH


/**
 * TinyFrame protocol library
 *
 * (c) Ondřej Hruška 2017-2018, MIT License
 * no liability/warranty, free for any use, must retain this notice & license
 *
 * Upstream URL: https://github.com/MightyPork/TinyFrame
 */


//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------

#include "TinyFrame_Types.hpp"
#include "TinyFrame_CRC.hpp"

namespace TinyFrame_n{

#define TF_VERSION "2.3.0"

/** TinyFrame class */

// Checksum type. Options:
//   CKSUM_NONE, CKSUM_XOR, CKSUM_CRC8, CKSUM_CRC16, CKSUM_CRC32
//   CKSUM_CUSTOM8, CKSUM_CUSTOM16, CKSUM_CUSTOM32
// Custom checksums require you to implement checksum functions (see TinyFrame.h)

#define TEMPLATE_ARGS CKSUM_t CKSUM_TYPE, size_t MAX_PAYLOAD_RX, size_t SENDBUF_LEN, size_t MAX_LISTENER
#define TEMPLATE_PARMS CKSUM_TYPE, MAX_PAYLOAD_RX, SENDBUF_LEN, MAX_LISTENER

template<
// Checksum Calculation Method
CKSUM_t CKSUM_TYPE=CKSUM_t::CRC8,
// Maximum received payload size (static buffer)
// Larger payloads will be rejected.
size_t MAX_PAYLOAD_RX=1024,
// Size of the sending buffer. Larger payloads will be split to pieces and sent
// in multiple calls to the write function. This can be lowered to reduce RAM usage.
size_t SENDBUF_LEN=128,
// Maximum number of listeners for each id, type or generic
size_t MAX_LISTENER=10
>
class TinyFrame{
    public:
        // forward declaration of constructor argument
        struct RequiredCallbacks;

        TinyFrame(const RequiredCallbacks& cb, const TinyFrameConfig_t& config, const Peer peer = Peer::SLAVE) : 
                    tfCallbacks_Required(cb), // copy callback struct
                    tfCallbacks_Optional({}), // zero initialization
                    tfConfig(config), // copy config struct
                    internal({}) // zero initialization
        { 
            this->internal.peer_bit = peer;
        };
        TinyFrame(const RequiredCallbacks& cb, const Peer peer = Peer::SLAVE) : TinyFrame(cb, CONFIG_DEFAULT, peer){ };

        ~TinyFrame() = default;

        /* --- Callback types --- */
        // This is publicly visible only to allow static init.
        /**
         * TinyFrame Type Listener callback
         *
         * @param tf - instance
         * @param msg - the received message, userdata is populated inside the object
         * @return listener result
         */
        typedef Result (*Listener)(Msg *msg);

        /**
         * TinyFrame Type Listener callback
         *
         * @param tf - instance
         * @param msg - the received message, userdata is populated inside the object
         * @return listener result
         */
        typedef Result (*Listener_Timeout)();

        // ------------------------ TO BE IMPLEMENTED BY USER ------------------------

        const struct RequiredCallbacks{
            /**
             * 'Write bytes' function that sends data to UART
             *
             * ! Implement this in your application code !
             */
            void (*WriteImpl)(const uint8_t *buff, uint32_t len);

            void (*Error)(std::string message);

        }tfCallbacks_Required; // variable definition

        const struct OptionalCallbacks{

            /** Claim the TX interface before composing and sending a frame */
            bool (*ClaimTx)();

            /** Free the TX interface after composing and sending a frame */
            void (*ReleaseTx)();

        }tfCallbacks_Optional; // variable definition

        struct IdListener_ {
            ID id;
            Listener fn;
            Listener_Timeout fn_timeout;
            TICKS timeout;     // nr of ticks remaining to disable this listener
            TICKS timeout_max; // the original timeout is stored here (0 = no timeout)
            void *userdata;
            void *userdata2;
        };

        struct TypeListener_ {
            TYPE type;
            Listener fn;
        };

        struct GenericListener_ {
            Listener fn;
        };

        const TinyFrameConfig_t tfConfig;

        struct{
            /* Public user data */
            void *userdata;
            uint32_t usertag;

            // --- the rest of the struct is internal, do not access directly ---

            /* Own state */
            Peer peer_bit;       //!< Own peer bit (unqiue to avoid msg ID clash)
            ID next_id;          //!< Next frame / frame chain ID

            /* Parser state */
            State_ state;
            TICKS parser_timeout_ticks;
            ID id;               //!< Incoming packet ID
            LEN len;             //!< Payload length
            uint8_t data[MAX_PAYLOAD_RX]; //!< Data byte buffer
            LEN rxi;             //!< Field size byte counter
            CKSUM<CKSUM_TYPE> cksum;         //!< Checksum calculated of the data stream
            CKSUM<CKSUM_TYPE> ref_cksum;     //!< Reference checksum read from the message
            TYPE type;           //!< Collected message type number
            bool discard_data;      //!< Set if (len > MAX_PAYLOAD) to read the frame, but ignore the data.

            /* Tx state */
            // Buffer for building frames
            uint8_t sendbuf[SENDBUF_LEN]; //!< Transmit temporary buffer

            uint32_t tx_pos;        //!< Next write position in the Tx buffer (used for multipart)
            uint32_t tx_len;        //!< Total expected Tx length
            CKSUM<CKSUM_TYPE> tx_cksum;      //!< Transmit checksum accumulator

            bool tfCallbacks_Optional_registered;
            bool soft_lock;         //!< Tx lock flag used if the mutex feature is not enabled.
                
            // Those counters are used to optimize look-up times.
            // They point to the highest used slot number,
            // or close to it, depending on the removal order.
            COUNT count_id_lst;
            COUNT count_type_lst;
            COUNT count_generic_lst;
            

            /* Transaction callbacks */
            IdListener_ id_listeners[MAX_LISTENER];
            TypeListener_ type_listeners[MAX_LISTENER];
            GenericListener_ generic_listeners[MAX_LISTENER];

        }internal;

        //---------------------------------------------------------------------------

        /**
         * Clear message struct
         *
         * @param msg - message to clear in-place
         */
        static inline void ClearMsg(Msg *msg)
        {
            msg = {};
        }

        // ---------------------------------- API CALLS --------------------------------------

        /**
         * Accept incoming bytes & parse frames
         *
         * @param buffer - byte buffer to process
         * @param count - nr of bytes in the buffer
         */
        void Accept(const uint8_t *buffer, uint32_t count);

        /**
         * Accept a single incoming byte
         *
         * @param c - a received char
         */
        void AcceptChar(uint8_t c);

        /**
         * This function should be called periodically.
         * The time base is used to time-out partial frames in the parser and
         * automatically reset it.
         * It's also used to expire ID listeners if a timeout is set when registering them.
         *
         * A common place to call this from is the SysTick handler.
         *
         */
        void Tick();

        /**
         * Reset the frame parser state machine.
         * This does not affect registered listeners.
         *
         */
        void ResetParser();


        // ---------------------------- MESSAGE LISTENERS -------------------------------

        /**
         * Register a frame type listener.
         *
         * @param msg - message (contains frame_id and userdata)
         * @param cb - callback
         * @param ftimeout - time out callback
         * @param timeout - timeout in ticks to auto-remove the listener (0 = keep forever)
         * @return slot index (for removing), or ERROR (-1)
         */
        bool AddIdListener(Msg *msg, Listener cb, Listener_Timeout ftimeout, TICKS timeout);

        /**
         * Remove a listener by the message ID it's registered for
         *
         * @param frame_id - the frame we're listening for
         */
        bool RemoveIdListener(ID frame_id);

        /**
         * Register a frame type listener.
         *
         * @param frame_type - frame type to listen for
         * @param cb - callback
         * @return slot index (for removing), or ERROR (-1)
         */
        bool AddTypeListener(TYPE frame_type, Listener cb);

        /**
         * Remove a listener by type.
         *
         * @param type - the type it's registered for
         */
        bool RemoveTypeListener(TYPE type);

        /**
         * Register a generic listener.
         *
         * @param cb - callback
         * @return slot index (for removing), or ERROR (-1)
         */
        bool AddGenericListener(Listener cb);

        /**
         * Remove a generic listener by function pointer
         *
         * @param cb - callback function to remove
         */
        bool RemoveGenericListener(Listener cb);

        /**
         * Renew an ID listener timeout externally (as opposed to by returning RENEW from the ID listener)
         *
         * @param id - listener ID to renew
         * @return true if listener was found and renewed
         */
        bool RenewIdListener(ID id);

        // ---------------------------- MUTEX FUNCTIONS ------------------------------

        bool RegisterMutex(OptionalCallbacks cbs);

        // ---------------------------- FRAME TX FUNCTIONS ------------------------------

        /**
         * Send a frame, no listener
         *
         * @param msg - message struct. ID is stored in the frame_id field
         * @return success
         */
        bool Send(Msg *msg);

        /**
         * Like Send, but without the struct
         */
        bool SendSimple(TYPE type, const uint8_t *data, LEN len);

        /**
         * Send a frame, and optionally attach an ID listener.
         *
         * @param msg - message struct. ID is stored in the frame_id field
         * @param listener - listener waiting for the response (can be nullptr)
         * @param ftimeout - time out callback
         * @param timeout - listener expiry time in ticks
         * @return success
         */
        bool Query(Msg *msg, Listener listener,
                    Listener_Timeout ftimeout, TICKS timeout);

        /**
         * Like Query(), but without the struct
         */
        bool QuerySimple(TYPE type,
                            const uint8_t *data, LEN len,
                            Listener listener, Listener_Timeout ftimeout, TICKS timeout);

        /**
         * Send a response to a received message.
         *
         * @param msg - message struct. ID is read from frame_id. set ->renew to reset listener timeout
         * @return success
         */
        bool Respond(Msg *msg);


        // ------------------------ MULTIPART FRAME TX FUNCTIONS -----------------------------
        // Those routines are used to send long frames without having all the data available
        // at once (e.g. capturing it from a peripheral or reading from a large memory buffer)

        /**
         * Send() with multipart payload.
         * msg.data is ignored and set to nullptr
         */
        bool Send_Multipart(Msg *msg);

        /**
         * SendSimple() with multipart payload.
         */
        bool SendSimple_Multipart(TYPE type, LEN len);

        /**
         * QuerySimple() with multipart payload.
         */
        bool QuerySimple_Multipart(TYPE type, LEN len, Listener listener, Listener_Timeout ftimeout, TICKS timeout);

        /**
         * Query() with multipart payload.
         * msg.data is ignored and set to nullptr
         */
        bool Query_Multipart(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout);

        /**
         * Respond() with multipart payload.
         * msg.data is ignored and set to nullptr
         */
        void Respond_Multipart(Msg *msg);

        /**
         * Send the payload for a started multipart frame. This can be called multiple times
         * if needed, until the full length is transmitted.
         *
         * @param buff - buffer to send bytes from
         * @param length - number of bytes to send
         */
        void Multipart_Payload(const uint8_t *buff, uint32_t length);

        /**
         * Close the multipart message, generating chekcsum and releasing the Tx lock.
         *
         */
        void Multipart_Close();


    private: 

        void _FN HandleReceivedMessage();
        uint32_t _FN ComposeHead(uint8_t *outbuff, Msg *msg);
        uint32_t _FN ComposeBody(uint8_t *outbuff,
                                            const uint8_t *data, LEN data_len,
                                            CKSUM<CKSUM_TYPE> *cksum);
        uint32_t _FN ComposeTail(uint8_t *outbuff, CKSUM<CKSUM_TYPE> *cksum);
        bool _FN SendFrame_Begin(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout);
        void _FN SendFrame_Chunk(const uint8_t *buff, uint32_t length);
        void _FN SendFrame_End();
        bool _FN SendFrame(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout);

        bool _FN ClaimTx_Internal(void);
        void _FN ReleaseTx_Internal(void);

        static void _FN renew_id_listener(IdListener_ *lst);
        void _FN cleanup_id_listener(COUNT i, IdListener_ *lst);
        void _FN cleanup_type_listener(COUNT i, TypeListener_ *lst);
        void _FN cleanup_generic_listener(COUNT i, GenericListener_ *lst);

        void _FN pars_begin_frame();

}; // class TinyFrame

/**
 * Frame parser internal state.
 */

// Helper macros
#define MIN(a, b) ((a)<(b)?(a):(b))
#define TRY(func) do { if(!(func)) return false; } while (0)


// Type-dependent masks for bit manipulation in the ID field
#define ID_MASK (ID)(((ID)1 << (sizeof(ID)*8 - 1)) - 1)
#define ID_PEERBIT (ID)((ID)1 << ((sizeof(ID)*8) - 1))

// Not thread safe lock implementation, used if user did not provide a better one.
// This is less reliable than a real mutex, but will catch most bugs caused by
// inappropriate use fo the API.

/** Claim the TX interface before composing and sending a frame */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::ClaimTx_Internal() {
    if (this->internal.soft_lock) {
        this->tfCallbacks_Required.Error("TF already locked for tx!");
        return false;
    }

    this->internal.soft_lock = true;
    return true;
}

/** Free the TX interface after composing and sending a frame */
template<TEMPLATE_ARGS>
void TinyFrame<TEMPLATE_PARMS>::ReleaseTx_Internal()
{
    this->internal.soft_lock = false;
}

//region Listeners

/** Reset ID listener's timeout to the original value */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::renew_id_listener(IdListener_ *lst)
{
    lst->timeout = lst->timeout_max;
}

/** Notify callback about ID listener's demise & let it free any resources in userdata */
template <TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::cleanup_id_listener(COUNT i, IdListener_ *lst)
{
    Msg msg;
    if (lst->fn == nullptr) return;

    // Make user clean up their data - only if not nullptr
    if (lst->userdata != nullptr || lst->userdata2 != nullptr) {
        msg.userdata = lst->userdata;
        msg.userdata2 = lst->userdata2;
        msg.data = nullptr; // this is a signal that the listener should clean up
        lst->fn(&msg); // return value is ignored here - use STAY or CLOSE
    }

    lst->fn = nullptr; // Discard listener
    lst->fn_timeout = nullptr;

    if (i == this->internal.count_id_lst - 1) {
        this->internal.count_id_lst--;
    }
}

/** Clean up Type listener */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::cleanup_type_listener(COUNT i, TypeListener_ *lst)
{
    lst->fn = nullptr; // Discard listener
    if (i == this->internal.count_type_lst - 1) {
        this->internal.count_type_lst--;
    }
}

/** Clean up Generic listener */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::cleanup_generic_listener(COUNT i, GenericListener_ *lst)
{
    lst->fn = nullptr; // Discard listener
    if (i == this->internal.count_generic_lst - 1) {
        this->internal.count_generic_lst--;
    }
}

/** Add a new ID listener. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::AddIdListener(Msg *msg, Listener cb, Listener_Timeout ftimeout, TICKS timeout)
{
    COUNT i;
    IdListener_ *lst;
    for (i = 0; i < MAX_LISTENER; i++) {
        lst = &this->internal.id_listeners[i];
        // test for empty slot
        if (lst->fn == nullptr) {
            lst->fn = cb;
            lst->fn_timeout = ftimeout;
            lst->id = msg->frame_id;
            lst->userdata = msg->userdata;
            lst->userdata2 = msg->userdata2;
            lst->timeout_max = lst->timeout = timeout;
            if (i >= this->internal.count_id_lst) {
                this->internal.count_id_lst = (COUNT) (i + 1);
            }
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Failed to add ID listener");
    return false;
}

/** Add a new Type listener. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::AddTypeListener(TYPE frame_type, Listener cb)
{
    COUNT i;
    TypeListener_ *lst;
    for (i = 0; i < MAX_LISTENER; i++) {
        lst = &this->internal.type_listeners[i];
        // test for empty slot
        if (lst->fn == nullptr) {
            lst->fn = cb;
            lst->type = frame_type;
            if (i >= this->internal.count_type_lst) {
                this->internal.count_type_lst = (COUNT) (i + 1);
            }
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Failed to add type listener");
    return false;
}

/** Add a new Generic listener. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::AddGenericListener(Listener cb)
{
    COUNT i;
    GenericListener_ *lst;
    for (i = 0; i < MAX_LISTENER; i++) {
        lst = &this->internal.generic_listeners[i];
        // test for empty slot
        if (lst->fn == nullptr) {
            lst->fn = cb;
            if (i >= this->internal.count_generic_lst) {
                this->internal.count_generic_lst = (COUNT) (i + 1);
            }
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Failed to add generic listener");
    return false;
}

/** Remove a ID listener by its frame ID. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::RemoveIdListener(ID frame_id)
{
    COUNT i;
    IdListener_ *lst;
    for (i = 0; i < this->internal.count_id_lst; i++) {
        lst = &this->internal.id_listeners[i];
        // test if live & matching
        if (lst->fn != nullptr && lst->id == frame_id) {
            this->cleanup_id_listener(i, lst);
            return true;
        }
    }

    this->tfCallbacks_Required.Error("ID listener %d to remove not found", (int)frame_id);
    return false;
}

/** Remove a type listener by its type. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::RemoveTypeListener(TYPE type)
{
    COUNT i;
    TypeListener_ *lst;
    for (i = 0; i < this->internal.count_type_lst; i++) {
        lst = &this->internal.type_listeners[i];
        // test if live & matching
        if (lst->fn != nullptr    && lst->type == type) {
            this->cleanup_type_listener(i, lst);
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Type listener %d to remove not found", (int)type);
    return false;
}

/** Remove a generic listener by its function pointer. Returns 1 on success. */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::RemoveGenericListener(Listener cb)
{
    COUNT i;
    GenericListener_ *lst;
    for (i = 0; i < this->internal.count_generic_lst; i++) {
        lst = &this->internal.generic_listeners[i];
        // test if live & matching
        if (lst->fn == cb) {
            this->cleanup_generic_listener(i, lst);
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Generic listener to remove not found");
    return false;
}

/** Handle a message that was just collected & verified by the parser */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::HandleReceivedMessage()
{
    COUNT i;
    IdListener_ *ilst;
    TypeListener_ *tlst;
    GenericListener_ *glst;
    Result res;

    // Prepare message object
    Msg msg;
    ClearMsg(&msg);
    msg.frame_id = this->internal.id;
    msg.is_response = false;
    msg.type = this->internal.type;
    msg.data = this->internal.data;
    msg.len = this->internal.len;

    // Any listener can consume the message, or let someone else handle it.

    // The loop upper bounds are the highest currently used slot index
    // (or close to it, depending on the order of listener removals).

    // ID listeners first
    for (i = 0; i < this->internal.count_id_lst; i++) {
        ilst = &this->internal.id_listeners[i];

        if (ilst->fn && ilst->id == msg.frame_id) {
            msg.userdata = ilst->userdata; // pass userdata pointer to the callback
            msg.userdata2 = ilst->userdata2;
            res = ilst->fn(&msg);
            ilst->userdata = msg.userdata; // put it back (may have changed the pointer or set to nullptr)
            ilst->userdata2 = msg.userdata2; // put it back (may have changed the pointer or set to nullptr)

            if (res != Result::NEXT) {
                // if it's CLOSE, we assume user already cleaned up userdata
                if (res == Result::RENEW) {
                    this->renew_id_listener(ilst);
                }
                else if (res == Result::CLOSE) {
                    // Set userdata to nullptr to avoid calling user for cleanup
                    ilst->userdata = nullptr;
                    ilst->userdata2 = nullptr;
                    this->cleanup_id_listener(i, ilst);
                }
                return;
            }
        }
    }
    // clean up for the following listeners that don't use userdata (this avoids data from
    // an ID listener that returned NEXT from leaking into Type and Generic listeners)
    msg.userdata = nullptr;
    msg.userdata2 = nullptr;

    // Type listeners
    for (i = 0; i < this->internal.count_type_lst; i++) {
        tlst = &this->internal.type_listeners[i];

        if (tlst->fn && tlst->type == msg.type) {
            res = tlst->fn(&msg);

            if (res != Result::NEXT) {
                // type listeners don't have userdata.
                // RENEW doesn't make sense here because type listeners don't expire = same as STAY

                if (res == Result::CLOSE) {
                    this->cleanup_type_listener(i, tlst);
                }
                return;
            }
        }
    }

    // Generic listeners
    for (i = 0; i < this->internal.count_generic_lst; i++) {
        glst = &this->internal.generic_listeners[i];

        if (glst->fn) {
            res = glst->fn(&msg);

            if (res != Result::NEXT) {
                // generic listeners don't have userdata.
                // RENEW doesn't make sense here because generic listeners don't expire = same as STAY

                // note: It's not expected that user will have multiple generic listeners, or
                // ever actually remove them. They're most useful as default callbacks if no other listener
                // handled the message.

                if (res == Result::CLOSE) {
                    this->cleanup_generic_listener(i, glst);
                }
                return;
            }
        }
    }

    this->tfCallbacks_Required.Error("Unhandled message, type");
}

template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::RegisterMutex(OptionalCallbacks cbs){
    bool success = false;
    if((cbs.ClaimTx != nullptr) && (cbs.ReleaseTx != nullptr)){
        this->tfCallbacks_Optional = cbs;
        this->internal.tfCallbacks_Optional_registered = true;
    }
    return success;
}

/** Externally renew an ID listener */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::RenewIdListener(ID id)
{
    COUNT i;
    IdListener_ *lst;
    for (i = 0; i < this->internal.count_id_lst; i++) {
        lst = &this->internal.id_listeners[i];
        // test if live & matching
        if (lst->fn != nullptr && lst->id == id) {
            renew_id_listener(lst);
            return true;
        }
    }

    this->tfCallbacks_Required.Error("Renew listener: not found (id %d)", (int)id);
    return false;
}

//endregion Listeners


//region Parser

/** Handle a received byte buffer */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::Accept(const uint8_t *buffer, uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        this->AcceptChar(buffer[i]);
    }
}

/** Reset the parser's internal state. */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::ResetParser()
{
    this->internal.state = State_::TFState_SOF;
    // more init will be done by the parser when the first byte is received
}

/** SOF was received - prepare for the frame */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::pars_begin_frame() {
    // Reset state vars
    CKSUM_RESET(this->internal.cksum);
    if(this->tfConfig.USE_SOF_BYTE){
        CKSUM_ADD(this->internal.cksum, this->tfConfig.SOF_BYTE);
    }

    this->internal.discard_data = false;

    // Enter ID state
    this->internal.state = State_::TFState_ID;
    this->internal.rxi = 0;
}

/** Handle a received char - here's the main state machine */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::AcceptChar(unsigned char c)
{
    // Parser timeout - clear
    if (this->internal.parser_timeout_ticks >= this->tfConfig.PARSER_TIMEOUT_TICKS) {
        if (this->internal.state != State_::TFState_SOF) {
            this->ResetParser();
            this->tfCallbacks_Required.Error("Parser timeout");
        }
    }
    this->internal.parser_timeout_ticks = 0;

// DRY snippet - collect multi-byte number from the input stream, byte by byte
// This is a little dirty, but makes the code easier to read. It's used like e.g. if(),
// the body is run only after the entire number (of data type 'type') was received
// and stored to 'dest'
#define COLLECT_NUMBER(dest, type, typesize) dest = (type)(((dest) << 8) | c); \
                                   if (++this->internal.rxi == typesize)

    if(!this->tfConfig.USE_SOF_BYTE){
        if (this->internal.state == State_::TFState_SOF) {
            this->pars_begin_frame();
        }
    }

    //@formatter:off
    switch (this->internal.state) {
        case State_::TFState_SOF:
            if (c == this->tfConfig.SOF_BYTE) {
                this->pars_begin_frame();
            }
            break;

        case State_::TFState_ID:
            CKSUM_ADD(this->internal.cksum, c);
            COLLECT_NUMBER(this->internal.id, ID, this->tfConfig.ID_BYTES) {
                // Enter LEN state
                this->internal.state = State_::TFState_LEN;
                this->internal.rxi = 0;
            }
            break;

        case State_::TFState_LEN:
            CKSUM_ADD(this->internal.cksum, c);
            COLLECT_NUMBER(this->internal.len, LEN, this->tfConfig.LEN_BYTES) {
                // Enter TYPE state
                this->internal.state = State_::TFState_TYPE;
                this->internal.rxi = 0;
            }
            break;

        case State_::TFState_TYPE:
            CKSUM_ADD(this->internal.cksum, c);
            COLLECT_NUMBER(this->internal.type, TYPE, this->tfConfig.TYPE_BYTES) {
                if(CKSUM_TYPE == CKSUM_t::NONE){
                    this->internal.state = State_::TFState_DATA;
                    this->internal.rxi = 0;
                }else{
                    // enter HEAD_CKSUM state
                    this->internal.state = State_::TFState_HEAD_CKSUM;
                    this->internal.rxi = 0;
                    this->internal.ref_cksum = 0;
                }
            }
            break;

        case State_::TFState_HEAD_CKSUM:
            COLLECT_NUMBER(this->internal.ref_cksum, CKSUM<CKSUM_TYPE>, sizeof(CKSUM<CKSUM_TYPE>)) {
                // Check the header checksum against the computed value
                CKSUM_FINALIZE(this->internal.cksum);

                if (this->internal.cksum != this->internal.ref_cksum) {
                    this->tfCallbacks_Required.Error("Rx head cksum mismatch");
                    this->ResetParser();
                    break;
                }

                if (this->internal.len == 0) {
                    // if the message has no body, we're done.
                    this->HandleReceivedMessage();
                    this->ResetParser();
                    break;
                }

                // Enter DATA state
                this->internal.state = State_::TFState_DATA;
                this->internal.rxi = 0;

                CKSUM_RESET(this->internal.cksum); // Start collecting the payload

                //Start collecting the payload
                if (this->internal.len > MAX_PAYLOAD_RX) {
                    this->tfCallbacks_Required.Error("Rx payload too long");
                    // ERROR - frame too long. Consume, but do not store.
                    this->internal.discard_data = true;
                }
            }
            break;

        case State_::TFState_DATA:
            if (this->internal.discard_data) {
                this->internal.rxi++;
            } else {
                CKSUM_ADD(this->internal.cksum, c);
                this->internal.data[this->internal.rxi++] = c;
            }

            if (this->internal.rxi == this->internal.len) {
                if(CKSUM_TYPE == CKSUM_t::NONE){
                    // All done
                    this->HandleReceivedMessage();
                    this->ResetParser();
                }else{
                    // Enter DATA_CKSUM state
                    this->internal.state = State_::TFState_DATA_CKSUM;
                    this->internal.rxi = 0;
                    this->internal.ref_cksum = 0;
                }
            }
            break;

        case State_::TFState_DATA_CKSUM:
            COLLECT_NUMBER(this->internal.ref_cksum, CKSUM<CKSUM_TYPE>, sizeof(CKSUM<CKSUM_TYPE>)) {
                // Check the header checksum against the computed value
                CKSUM_FINALIZE(this->internal.cksum);
                if (!this->internal.discard_data) {
                    if (this->internal.cksum == this->internal.ref_cksum) {
                        this->HandleReceivedMessage();
                    } else {
                        this->tfCallbacks_Required.Error("Body cksum mismatch");
                    }
                }

                this->ResetParser();
            }
            break;
    }
    //@formatter:on
}

//endregion Parser


//region Compose and send

// Helper macros for the Compose functions
// use variables: si - signed int, b - byte, outbuff - target buffer, pos - count of bytes in buffer


/**
 * Write a number to the output buffer.
 *
 * @param type - data type
 * @param num - number to write
 * @param xtra - extra callback run after each byte, 'b' now contains the byte.
 */
#define WRITENUM_BASE(typesize, num, xtra) \
    for (si = (typesize-1); si>=0; si--) { \
        b = (uint8_t)((num) >> (si*8) & 0xFF); \
        outbuff[pos++] = b; \
        xtra; \
    }

/**
 * Do nothing
 */
#define _NOOP()

/**
 * Write a number without adding its bytes to the checksum
 *
 * @param type - data type
 * @param num - number to write
 */
#define WRITENUM(typesize, num)       WRITENUM_BASE(typesize, num, _NOOP())

/**
 * Write a number AND add its bytes to the checksum
 *
 * @param type - data type
 * @param num - number to write
 */
#define WRITENUM_CKSUM(cksumtype, typesize, num) WRITENUM_BASE(typesize, num, CKSUM_ADD(cksum, b))

/**
 * Compose a frame (used internally by Send and Respond).
 * The frame can be sent using WriteImpl(), or received by Accept()
 *
 * @param outbuff - buffer to store the result in
 * @param msg - message written to the buffer
 * @return nr of bytes in outbuff used by the frame, 0 on failure
 */
template<TEMPLATE_ARGS>
uint32_t _FN TinyFrame<TEMPLATE_PARMS>::ComposeHead(uint8_t *outbuff, Msg *msg)
{
    int8_t si = 0; // signed small int
    uint8_t b = 0;
    ID id = 0;
    CKSUM<CKSUM_TYPE> cksum = 0;
    uint32_t pos = 0;

    (void)cksum; // suppress "unused" warning if checksums are disabled

    CKSUM_RESET(cksum);

    // Gen ID
    if (msg->is_response) {
        id = msg->frame_id;
    }
    else {
        id = (ID) (this->internal.next_id++ & ID_MASK);
        if (this->internal.peer_bit != Peer::SLAVE) {
            id |= ID_PEERBIT;
        }
    }

    msg->frame_id = id; // put the resolved ID into the message object for later use

    // --- Start ---
    CKSUM_RESET(cksum);
    if(this->tfConfig.TYPE_BYTES){
        outbuff[pos++] = this->tfConfig.SOF_BYTE;
        CKSUM_ADD(cksum, this->tfConfig.SOF_BYTE);
    }

    WRITENUM_CKSUM(CKSUM_TYPE, this->tfConfig.ID_BYTES, id);
    WRITENUM_CKSUM(CKSUM_TYPE, this->tfConfig.LEN_BYTES, msg->len);
    WRITENUM_CKSUM(CKSUM_TYPE, this->tfConfig.TYPE_BYTES, msg->type);
    if(CKSUM_TYPE != CKSUM_t::NONE){
        CKSUM_FINALIZE(cksum);
        WRITENUM(sizeof(CKSUM<CKSUM_TYPE>), cksum);
    }

    return pos;
}

/**
 * Compose a frame (used internally by Send and Respond).
 * The frame can be sent using WriteImpl(), or received by Accept()
 *
 * @param outbuff - buffer to store the result in
 * @param data - data buffer
 * @param data_len - data buffer len
 * @param cksum - checksum variable, used for all calls to ComposeBody. Must be reset before first use! (CKSUM_RESET(cksum);)
 * @return nr of bytes in outbuff used
 */
template<TEMPLATE_ARGS>
uint32_t _FN TinyFrame<TEMPLATE_PARMS>::ComposeBody(uint8_t *outbuff,
                                    const uint8_t *data, LEN data_len,
                                    CKSUM<CKSUM_TYPE> *cksum)
{
    LEN i = 0;
    uint8_t b = 0;
    uint32_t pos = 0;

    for (i = 0; i < data_len; i++) {
        b = data[i];
        outbuff[pos++] = b;
        CKSUM_ADD(*cksum, b);
    }

    return pos;
}

/**
 * Finalize a frame
 *
 * @param outbuff - buffer to store the result in
 * @param cksum - checksum variable used for the body
 * @return nr of bytes in outbuff used
 */
template<TEMPLATE_ARGS>
uint32_t _FN TinyFrame<TEMPLATE_PARMS>::ComposeTail(uint8_t *outbuff, CKSUM<CKSUM_TYPE> *cksum)
{
    int8_t si = 0; // signed small int
    uint8_t b = 0;
    uint32_t pos = 0;

    if(CKSUM_TYPE != CKSUM_t::NONE){
        CKSUM_FINALIZE(*cksum);
        WRITENUM(sizeof(CKSUM<CKSUM_TYPE>), *cksum);
    }
    return pos;
}

/**
 * Begin building and sending a frame
 *
 * @param msg - message to send
 * @param listener - response listener or nullptr
 * @param ftimeout - time out callback
 * @param timeout - listener timeout ticks, 0 = indefinite
 * @return success (mutex claimed and listener added, if any)
 */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::SendFrame_Begin(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    if(this->tfCallbacks_Optional.ClaimTx != nullptr){
        TRY(this->tfCallbacks_Optional.ClaimTx());
    }else{
        TRY(this->ClaimTx_Internal());
    }

    this->internal.tx_pos = (uint32_t) this->ComposeHead(this->internal.sendbuf, msg); // frame ID is incremented here if it's not a response
    this->internal.tx_len = msg->len;

    if (listener) {
        if(!this->AddIdListener(msg, listener, ftimeout, timeout)) {
            if(this->tfCallbacks_Optional.ReleaseTx != nullptr){
                TRY(this->tfCallbacks_Optional.ClaimTx());
            }else{
                TRY(this->ClaimTx_Internal());
            }
            return false;
        }
    }

    CKSUM_RESET(this->internal.tx_cksum);
    return true;
}

/**
 * Build and send a part (or all) of a frame body.
 * Caution: this does not check the total length against the length specified in the frame head
 *
 * @param buff - bytes to write
 * @param length - count
 */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::SendFrame_Chunk(const uint8_t *buff, uint32_t length)
{
    uint32_t remain;
    uint32_t chunk;
    uint32_t sent = 0;

    remain = length;
    while (remain > 0) {
        // Write what can fit in the tx buffer
        chunk = MIN(SENDBUF_LEN - this->internal.tx_pos, remain);
        this->internal.tx_pos += ComposeBody(this->internal.sendbuf+this->internal.tx_pos, buff+sent, (LEN) chunk, &this->internal.tx_cksum);
        remain -= chunk;
        sent += chunk;

        // Flush if the buffer is full
        if (this->internal.tx_pos == SENDBUF_LEN) {
            this->tfCallbacks_Required.WriteImpl((const uint8_t *) this->internal.sendbuf, this->internal.tx_pos);
            this->internal.tx_pos = 0;
        }
    }
}

/**
 * End a multi-part frame. This sends the checksum and releases mutex.
 *
 */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::SendFrame_End()
{
    // Checksum only if message had a body
    if (this->internal.tx_len > 0) {
        // Flush if checksum wouldn't fit in the buffer
        if (SENDBUF_LEN - this->internal.tx_pos < sizeof(CKSUM<CKSUM_TYPE>)) {
            this->tfCallbacks_Required.WriteImpl((const uint8_t *) this->internal.sendbuf, this->internal.tx_pos);
            this->internal.tx_pos = 0;
        }

        // Add checksum, flush what remains to be sent
        this->internal.tx_pos += ComposeTail(this->internal.sendbuf + this->internal.tx_pos, &this->internal.tx_cksum);
    }

    this->tfCallbacks_Required.WriteImpl((const uint8_t *) this->internal.sendbuf, this->internal.tx_pos);
    this->tfCallbacks_Optional.ReleaseTx();
}

/**
 * Send a message
 *
 * @param msg - message object
 * @param listener - ID listener, or nullptr
 * @param ftimeout - time out callback
 * @param timeout - listener timeout, 0 is none
 * @return true if sent
 */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::SendFrame(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    TRY(this->SendFrame_Begin(msg, listener, ftimeout, timeout));
    if (msg->len == 0 || msg->data != nullptr) {
        // Send the payload and checksum only if we're not starting a multi-part frame.
        // A multi-part frame is identified by passing nullptr to the data field and setting the length.
        // User then needs to call those functions manually
        this->SendFrame_Chunk(msg->data, msg->len);
        this->SendFrame_End();
    }
    return true;
}

//endregion Compose and send


//region Sending API funcs

/** send without listener */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::Send(Msg *msg)
{
    return this->SendFrame(msg, nullptr, nullptr, 0);
}

/** send without listener and struct */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::SendSimple(TYPE type, const uint8_t *data, LEN len)
{
    Msg msg;
    ClearMsg(&msg);
    msg.type = type;
    msg.data = data;
    msg.len = len;
    return this->Send(&msg);
}

/** send with a listener waiting for a reply, without the struct */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::QuerySimple(TYPE type, const uint8_t *data, LEN len, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    Msg msg;
    ClearMsg(&msg);
    msg.type = type;
    msg.data = data;
    msg.len = len;
    return this->SendFrame(&msg, listener, ftimeout, timeout);
}

/** send with a listener waiting for a reply */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::Query(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    return this->SendFrame(msg, listener, ftimeout, timeout);
}

/** Like Send, but with explicit frame ID (set inside the msg object), use for responses */
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::Respond(Msg *msg)
{
    msg->is_response = true;
    return this->Send(msg);
}

//endregion Sending API funcs


//region Sending API funcs - multipart
template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::Send_Multipart(Msg *msg)
{
    msg->data = nullptr;
    return this->Send(msg);
}

template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::SendSimple_Multipart(TYPE type, LEN len)
{
    return this->SendSimple(type, nullptr, len);
}

template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::QuerySimple_Multipart(TYPE type, LEN len, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    return this->QuerySimple(type, nullptr, len, listener, ftimeout, timeout);
}

template<TEMPLATE_ARGS>
bool _FN TinyFrame<TEMPLATE_PARMS>::Query_Multipart(Msg *msg, Listener listener, Listener_Timeout ftimeout, TICKS timeout)
{
    msg->data = nullptr;
    return this->Query(msg, listener, ftimeout, timeout);
}

template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::Respond_Multipart(Msg *msg)
{
    msg->data = nullptr;
    this->Respond(msg);
}

template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::Multipart_Payload(const uint8_t *buff, uint32_t length)
{
    this->SendFrame_Chunk(buff, length);
}

template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::Multipart_Close()
{
    this->SendFrame_End();
}

//endregion Sending API funcs - multipart


/** Timebase hook - for timeouts */
template<TEMPLATE_ARGS>
void _FN TinyFrame<TEMPLATE_PARMS>::Tick()
{
    COUNT i;
    IdListener_ *lst;

    // increment parser timeout (timeout is handled when receiving next byte)
    if (this->internal.parser_timeout_ticks < this->tfConfig.PARSER_TIMEOUT_TICKS) {
        this->internal.parser_timeout_ticks++;
    }

    // decrement and expire ID listeners
    for (i = 0; i < this->internal.count_id_lst; i++) {
        lst = &this->internal.id_listeners[i];
        if (!lst->fn || lst->timeout == 0) continue;
        // count down...
        if (--lst->timeout == 0) {
            this->tfCallbacks_Required.Error("ID listener has expired");
            if (lst->fn_timeout != nullptr) {
                lst->fn_timeout(); // execute timeout function
            }
            // Listener has expired
            this->cleanup_id_listener(i, lst);
        }
    }
}

} // TinyFrame_n

#endif // TinyFrameH
