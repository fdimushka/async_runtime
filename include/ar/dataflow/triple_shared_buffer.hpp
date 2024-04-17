#ifndef AR_DATAFLOW_TRIPLE_SHARED_BUFFER_H
#define AR_DATAFLOW_TRIPLE_SHARED_BUFFER_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer.hpp"

namespace AsyncRuntime::Dataflow {

    template< class T >
    class TripleBuffer : public SharedBuffer< T > {
    public:
        explicit TripleBuffer();

        TripleBuffer<T>(const TripleBuffer<T>&) = delete;
        TripleBuffer<T>& operator=(const TripleBuffer<T>&) = delete;

        ~TripleBuffer() override = default;
        SharedBufferError Write( T && msg ) override;
        SharedBufferError Write( const T & msg ) override;
        std::optional<T> Read() override;
        bool TryRead(T & res) override { return false; }
        bool Empty() override { return false; }
        void Flush() override { }
    private:
        bool IsNewWrite(uint8_t flags); // check if the newWrite bit is 1
        uint8_t SwapSnapWithClean(uint8_t flags); // swap Snap and Clean indexes
        uint8_t NewWriteSwapCleanWithDirty(uint8_t flags); // set

        // 8 bit flags are (unused) (new write) (2x dirty) (2x clean) (2x snap)
        // newWrite   = (flags & 0x40)
        // dirtyIndex = (flags & 0x30) >> 4
        // cleanIndex = (flags & 0xC) >> 2
        // snapIndex  = (flags & 0x3)
        mutable std::atomic<uint8_t> flags;

        T buffer[3];
    };

    template <typename T>
    TripleBuffer<T>::TripleBuffer() : SharedBuffer<T>(kTRIPLE_BUFFER) {
        flags.store(0x6, std::memory_order_relaxed); // initially dirty = 0, clean = 1 and snap = 2
    }


    template <typename T>
    std::optional<T> TripleBuffer<T>::Read() {
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        do {
            if( !IsNewWrite(flagsNow) )
                break;
        } while(!flags.compare_exchange_weak(flagsNow,
                                             SwapSnapWithClean(flagsNow),
                                             std::memory_order_release,
                                             std::memory_order_consume));

        return std::optional(buffer[flags.load(std::memory_order_consume) & 0x3]);
    }


    template <typename T>
    SharedBufferError TripleBuffer<T>::Write(T && item) {
        buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4] = std::move(item);
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        while(!flags.compare_exchange_weak(flagsNow,
                                           NewWriteSwapCleanWithDirty(flagsNow),
                                           std::memory_order_release,
                                           std::memory_order_consume));
        return SharedBufferError::kNO_ERROR;
    }

    template <typename T>
    SharedBufferError TripleBuffer<T>::Write(const T & item) {
        buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4] = item;
        uint8_t flagsNow(flags.load(std::memory_order_consume));
        while(!flags.compare_exchange_weak(flagsNow,
                                           NewWriteSwapCleanWithDirty(flagsNow),
                                           std::memory_order_release,
                                           std::memory_order_consume));
        return SharedBufferError::kNO_ERROR;
    }


    template <typename T>
    bool TripleBuffer<T>::IsNewWrite(uint8_t flags) {
        return ((flags & 0x40) != 0);
    }


    template <typename T>
    uint8_t TripleBuffer<T>::SwapSnapWithClean(uint8_t flags) {
        return (flags & 0x30) | ((flags & 0x3) << 2) | ((flags & 0xC) >> 2);
    }


    template <typename T>
    uint8_t TripleBuffer<T>::NewWriteSwapCleanWithDirty(uint8_t flags) {
        return 0x40 | ((flags & 0xC) << 2) | ((flags & 0x30) >> 2) | (flags & 0x3);
    }
}

#endif //AR_DATAFLOW_TRIPLE_SHARED_BUFFER_H
