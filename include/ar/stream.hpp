#ifndef AR_STREAM_HPP
#define AR_STREAM_HPP


#include "ar/task.hpp"
#include "uv.h"


namespace AsyncRuntime {
    struct IOStreamBuffer {
        IOStreamBuffer();
        IOStreamBuffer(const char* buf, int64_t len);
        ~IOStreamBuffer();


        uv_buf_t* Next(int64_t size = 1024);


        void Flush();
        int64_t                 seek = 0;
        int64_t                 length = 0;
        int64_t                 allocated_length = 0;
        char*                   buffer = nullptr;
        bool                    allocated = false;
        uv_buf_t                uv_buf;
    };


    class IOFsStream {
    public:
        IOFsStream();
        IOFsStream(const char* buf, int64_t len);
        ~IOFsStream();


        void Flush();


        /**
         * @brief set file descriptor
         * @param file - file descriptor
         */
        void SetFd(uv_file file) { fd = file; }


        /**
         * @brief get file descriptor
         * @return file descriptor
         */
        uv_file GetFd() const { return fd; }


        /**
         * @brief
         * @return
         */
        IOStreamBuffer &GetReadStream() { return read_stream; };
        IOStreamBuffer &GetWriteStream() { return write_stream; };


        /**
         * @brief get size of current read buffer
         * @return
         */
        int64_t GetReadBufferSize() const { return read_stream.length; };
        const char* GetReadBuffer() const { return read_stream.buffer; };
    private:
        uv_file                 fd;
        IOStreamBuffer          read_stream;
        IOStreamBuffer          write_stream;
    };

    typedef std::shared_ptr<IOFsStream>     IOFsStreamPtr;
}

#endif //AR_STREAM_HPP
