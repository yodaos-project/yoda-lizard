#pragma once

namespace rokid {
namespace lizard {

void set_rw_timeout(int socket, int32_t tm, bool rd);

#ifdef LIZARD_DEBUG
#include <stdint.h>
#include <stdio.h>
void print_hex_data(const uint8_t *data, uint32_t size);
#endif

} // namespace lizard
} // namespace rokid
