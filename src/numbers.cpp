#include "numbers.h"
#include <cassert>

using namespace AsyncRuntime;

uint32_t Numbers::Pack(uint16_t x, uint16_t y) {
    return ((uint32_t)x << 16) | (uint16_t)y;
}

void Numbers::Unpack(uint32_t v, uint16_t & x, uint16_t & y) {
    x = uint16_t(v >> 16);
    y = uint16_t(v);
}