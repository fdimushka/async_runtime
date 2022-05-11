#include "ar/stream.hpp"
#include <algorithm>
#include <numeric>


using namespace AsyncRuntime;


IOStream::IOStream() : fd(-1), seek(0), length(0), allocated_length(0), allocated(false) { }
IOStream::IOStream(const char *buf, int64_t len) : fd(-1), seek(0), length(len), allocated_length(len), allocated(true)
{
    assert(length > 0);
    assert(allocated_length > 0);
    assert(allocated_length == length);
    assert(buf != nullptr);

    //@todo add alignment
    buffer = (char*)malloc(sizeof(char*) * allocated_length);

    if(!buffer) {
        RNT_ASSERT_MSG( false, "memory out of bound");
    }

    memcpy(buffer, buf, sizeof(char*) * allocated_length);
}


IOStream::~IOStream()
{
    Flush();
}


void IOStream::Flush()
{
    if(buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }

    fd = -1;
    allocated_length = 0;
    length = 0;
    seek = 0;
}


void IOStream::SetMode(const Mode &mode)
{
    switch (mode) {
        case W: allocated = true; break;
        case R: allocated = false; break;
    }
}


uv_buf_t *IOStream::Next(int64_t size)
{
    if(!allocated) {
        seek = length;
        if (allocated_length < seek + size) {
            allocated_length = seek + size;

            //@todo add alignment
            buffer = (char *) realloc(buffer, sizeof(char *) * allocated_length);

            if (!buffer) {
                RNT_ASSERT_MSG(false, "memory out of bound");
            }
        }

        uv_buf.base = buffer + seek;
        uv_buf.len = size;

        memset(uv_buf.base, 0, uv_buf.len);
    }else{
        assert(length > 0);

        if(seek >= length)
            return nullptr;

        int64_t s = std::min(size, length - seek);
        uv_buf.base = buffer + seek;
        uv_buf.len = s;
        seek += s;
    }

    return &uv_buf;
}


bool IOStream::Next(uv_buf_t* buf, int64_t size) {
    if(!allocated) {
        seek = length;
        if (allocated_length < seek + size) {
            allocated_length = seek + size;

            //@todo add alignment
            buffer = (char *) realloc(buffer, sizeof(char *) * allocated_length);

            if (!buffer) {
                RNT_ASSERT_MSG(false, "memory out of bound");
            }
        }

        buf->base = buffer + seek;
        buf->len = size;

        memset(buf->base, 0, buf->len);
    }else{
        assert(length > 0);

        if(seek >= length)
            return false;

        int64_t s = std::min(size, length - seek);
        buf->base = buffer + seek;
        buf->len = s;
        seek += s;
    }

    return true;
}


void IOStream::Begin()
{
    seek = 0;
}

