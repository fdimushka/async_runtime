#ifndef AR_NUMBERS_H
#define AR_NUMBERS_H

#include <cstdint>

namespace AsyncRuntime::Numbers {

    /**
     * @brief
     * @param x
     * @param y
     * @return
     */
    uint32_t Pack(uint16_t x, uint16_t y);

    /**
     * @brief
     * @param v
     * @param x
     * @param y
     */
    void Unpack(uint32_t v, uint16_t & x, uint16_t & y);
}

#endif //AR_NUMBERS_H
