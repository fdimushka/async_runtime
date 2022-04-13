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
    class IOFsStream {
        friend class Task;

#ifdef USE_TESTS
        friend STREAM_TEST_FRIEND;
#endif

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


        friend void FsReadCb(uv_fs_s* req);
        friend void NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    private:
        uv_file                 fd;
        int64_t                 seek = 0;
        int64_t                 length = 0;
        int64_t                 allocated_length = 0;
        char*                   buffer = nullptr;
        bool                    allocated = false;
        uv_buf_t                uv_buf;
    };

    typedef std::shared_ptr<IOFsStream>     IOFsStreamPtr;


    inline IOFsStreamPtr MakeStream() {
        return std::make_shared<IOFsStream>();
    }


    inline IOFsStreamPtr MakeStream(const char *buffer, size_t length) {
        return std::make_shared<IOFsStream>(buffer, length);
    }
}

#endif //AR_STREAM_HPP
