#ifndef AR_STREAM_HPP
#define AR_STREAM_HPP


#include "ar/task.hpp"
#include "uv.h"


namespace AsyncRuntime {
    class IOFsStream {
    public:
        IOFsStream();
        ~IOFsStream();


        void Flush();
        uv_buf_t *CreateReadBuffer(size_t len = 1024);
        uv_buf_t *ReadBuffer();
        void IncreaseReadBufferLength(int64_t len);


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
         * @brief get size of current read buffer
         * @return
         */
        int64_t GetReadBufferSize() const;
    private:
        uv_file                 fd;
        std::vector<uv_buf_t>   read_bufs;
        int64_t                 read_buf_len;
    };


    typedef std::shared_ptr<IOFsStream>     IOFsStreamPtr;
}

#endif //AR_STREAM_HPP
