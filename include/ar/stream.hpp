#ifndef AR_STREAM_HPP
#define AR_STREAM_HPP


#include "ar/task.hpp"
#include "uv.h"


#ifdef USE_TESTS
class STREAM_TEST_FRIEND;
#endif


namespace AsyncRuntime {

    /**
     * @brief i/o file stream
     * @class IOFsStream
     */
    class IOStream {
        friend class Task;

#ifdef USE_TESTS
        friend STREAM_TEST_FRIEND;
#endif

    public:
        enum Mode { W,R };


        IOStream();
        IOStream(const char* buf, int64_t len);
        IOStream(const IOStream& other);
        IOStream(IOStream&& other) noexcept;
        ~IOStream();


        IOStream& operator=(const IOStream& other) noexcept;
        IOStream& operator=(IOStream&& other) noexcept;


        void Flush();


        /**
         * @brief
         * @param mode
         */
        void SetMode(const Mode& mode);


        /**
         * @brief
         * @param size
         * @return
         */
        uv_buf_t* Next(int64_t size = 1024);
        bool Next(uv_buf_t* buf, int64_t size = 1024);
        void Begin();


        /**
         * @brief get size of current read buffer
         * @return
         */
        int64_t GetBufferSize() const { return length; };
        const char* GetBuffer() const { return buffer; };


        void SetLength(int64_t l) { length = l;}
        int64_t GetLength() const { return length; };
    private:
        int64_t                 seek = 0;
        int64_t                 length = 0;
        int64_t                 allocated_length = 0;
        char*                   buffer = nullptr;
        bool                    allocated = false;
        uv_buf_t                uv_buf;
    };

    typedef std::shared_ptr<IOStream>     IOStreamPtr;


    inline IOStreamPtr MakeStream() {
        return std::make_shared<IOStream>();
    }


    inline IOStreamPtr MakeStream(const char *buffer, size_t length) {
        return std::make_shared<IOStream>(buffer, length);
    }
}

#endif //AR_STREAM_HPP
