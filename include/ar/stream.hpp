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
        ~IOStream();


        void Flush();


        /**
         * @brief set file descriptor
         * @param file - file descriptor
         */
        void SetFd(uv_file file) { fd = file; }


        /**
         * @brief
         * @param mode
         */
        void SetMode(const Mode& mode);


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

    typedef std::shared_ptr<IOStream>     IOStreamPtr;


    inline IOStreamPtr MakeStream() {
        return std::make_shared<IOStream>();
    }


    inline IOStreamPtr MakeStream(const char *buffer, size_t length) {
        return std::make_shared<IOStream>(buffer, length);
    }
}

#endif //AR_STREAM_HPP
