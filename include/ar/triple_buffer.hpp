#ifndef AR_TRIPLE_BUFFER_H
#define AR_TRIPLE_BUFFER_H

#include <iostream>
#include <map>
#include <atomic>


namespace AsyncRuntime {
    /**
     * @class lock-free triple buffer
     * @tparam T
     */
    template <typename T>
    class TripleBuffer {
    public:
        TripleBuffer<T>();
        TripleBuffer<T>(const TripleBuffer<T>&) = delete;
        TripleBuffer<T>& operator=(const TripleBuffer<T>&) = delete;


        /**
         * @brief wrapper to read the last available element
         * @return T
         */
        T read();


        /**
         * @brief wrapper to update with a new element
         * @param newT
         */
        void write(const T& item);


        /**
         * @brief is exist new data for read
         * @return
         */
        bool isUpdate();
    private:
        bool isNewWrite(uint8_t flags); // check if the newWrite bit is 1
        uint8_t swapSnapWithClean(uint8_t flags); // swap Snap and Clean indexes
        uint8_t newWriteSwapCleanWithDirty(uint8_t flags); // set

        // 8 bit flags are (unused) (new write) (2x dirty) (2x clean) (2x snap)
        // newWrite   = (flags & 0x40)
        // dirtyIndex = (flags & 0x30) >> 4
        // cleanIndex = (flags & 0xC) >> 2
        // snapIndex  = (flags & 0x3)
        mutable std::atomic<uint8_t> flags;

        T buffer[3];
    };


    template <typename T>
    TripleBuffer<T>::TripleBuffer()
    {
        T dummy = T();
        buffer[0] = dummy;
        buffer[1] = dummy;
        buffer[2] = dummy;
        flags.store(0x6, std::memory_order_relaxed); // initially dirty = 0, clean = 1 and snap = 2
    }


    template <typename T>
    T TripleBuffer<T>::read(){
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        do {
            if( !isNewWrite(flagsNow) )
                break;
        } while(!flags.compare_exchange_weak(flagsNow,
                                             swapSnapWithClean(flagsNow),
                                             std::memory_order_release,
                                             std::memory_order_consume));

        return buffer[flags.load(std::memory_order_consume) & 0x3]; // return it
    }


    template <typename T>
    void TripleBuffer<T>::write(const T& item){
        buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4] = item;
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        while(!flags.compare_exchange_weak(flagsNow,
                                           newWriteSwapCleanWithDirty(flagsNow),
                                           std::memory_order_release,
                                           std::memory_order_consume));
    }


    template <typename T>
    bool TripleBuffer<T>::isUpdate() {
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        return isNewWrite(flagsNow);
    }


    template <typename T>
    bool TripleBuffer<T>::isNewWrite(uint8_t flags){
        // check if the newWrite bit is 1
        return ((flags & 0x40) != 0);
    }


    template <typename T>
    uint8_t TripleBuffer<T>::swapSnapWithClean(uint8_t flags){
        // swap snap with clean
        return (flags & 0x30) | ((flags & 0x3) << 2) | ((flags & 0xC) >> 2);
    }


    template <typename T>
    uint8_t TripleBuffer<T>::newWriteSwapCleanWithDirty(uint8_t flags){
        // set newWrite bit to 1 and swap clean with dirty
        return 0x40 | ((flags & 0xC) << 2) | ((flags & 0x30) >> 2) | (flags & 0x3);
    }
}

#endif //AR_TRIPLE_BUFFER_H
