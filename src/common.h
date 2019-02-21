#pragma once

namespace rokid {
namespace lizard {

#ifdef LIZARD_DEBUG
#include <stdint.h>
#include <stdio.h>
void print_hex_data(const uint8_t *data, uint32_t size);
#endif

} // namespace lizard
} // namespace rokid
