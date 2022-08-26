//
// Created by MightyPork on 2017/10/15.
//

#ifndef DEMO_H
#define DEMO_H

#include <stdbool.h>
#include "../TinyFrame.hpp"
#include "utils.hpp"

using namespace TinyFrame_n;
using TinyFrame_Demo=TinyFrameDefault;

#define PORT 9798

extern TinyFrame_Demo *demo_tf;

extern void WriteImpl(const uint8_t *buff, uint32_t len);

extern void ErrorCallback(std::string message);

/** Sleep and wait for ^C */
void demo_sleep(void);

/** Init server - DOES NOT init TinyFrame! */
void demo_init(Peer peer);

/** Disconnect client from the server - can be called by a server-side callback */
void demo_disconn(void);

#endif //DEMO_H
