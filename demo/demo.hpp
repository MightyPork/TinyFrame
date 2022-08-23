//
// Created by MightyPork on 2017/10/15.
//

#ifndef DEMO_H
#define DEMO_H

#include <stdbool.h>
#include "../TinyFrame.hpp"
#include "utils.hpp"

#define PORT 9798

extern TinyFrame *demo_tf;

/** Sleep and wait for ^C */
void demo_sleep(void);

/** Init server - DOES NOT init TinyFrame! */
void demo_init(Peer peer);

/** Disconnect client from the server - can be called by a server-side callback */
void demo_disconn(void);

#endif //DEMO_H
