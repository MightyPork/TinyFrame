//
// Created by MightyPork on 2017/10/15.
//

#ifndef TF_DEMO_H
#define TF_DEMO_H

#include <stdbool.h>
#include "../TinyFrame.h"
#include "utils.h"

#define PORT 9798

/** Init server - DOES NOT init TinyFrame! */
void demo_init(TF_PEER peer);

/** Disconnect client from the server - claled by a server-side callback */
void demo_disconn(void);

#endif //TF_DEMO_H
