#pragma once

#ifdef LIZARD_DEBUG
#include <stdint.h>
#include <stdio.h>
#endif
#include "rlog.h"

namespace rokid {
namespace lizard {

#define TAG "lizard"

void set_rw_timeout(int socket, int32_t tm, bool rd);

#ifdef LIZARD_DEBUG
void print_hex_data(const uint8_t *data, uint32_t size);
#endif

} // namespace lizard
} // namespace rokid
