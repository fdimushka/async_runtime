#include "ar/stream.hpp"
#include <algorithm>
#include <numeric>
#include <cstring>


using namespace AsyncRuntime;


IOStream::IOStream() : seek(0), length(0), allocated_length(0), allocated(false), uv_buf{} { }
IOStream::IOStream(const char *buf, int64_t len) : seek(0), length(len), allocated_length(len), allocated(true), uv_buf{}
{
    assert(length > 0);
    assert(allocated_length > 0);
    assert(allocated_length == length);
    assert(buf != nullptr);

    //@todo add alignment
    buffer = (char*)malloc(allocated_length);

    if(!buffer) {
        RNT_ASSERT_MSG( false, "memory out of bound");
    }

    std::memcpy(buffer, buf, allocated_length);
}


IOStream::IOStream(const IOStream& other) :
    seek(other.seek),
    length(other.length),
    buffer(nullptr),
    allocated_length(other.allocated_length),
    allocated(other.allocated),
    uv_buf{}
{
    if(other.length > 0) {
        buffer = (char *) malloc(other.length);
        memcpy(buffer, other.buffer, other.length);
    }
}


IOStream::IOStream(IOStream&& other) noexcept :
    seek(other.seek),
    length(other.length),
    buffer(other.buffer),
    allocated_length(other.allocated_length),
    allocated(other.allocated),
    uv_buf{}
{
    other.uv_buf = {};
    other.seek = 0;
    other.length = 0;
    other.buffer = nullptr;
    other.allocated_length = 0;
}


IOStream::~IOStream()
{
    Flush();
}


IOStream& IOStream::operator=(IOStream&& other) noexcept {
    if (this != &other)
    {
        seek = other.seek;
        length = other.length;
        buffer = other.buffer;
        allocated_length = other.allocated_length;
        allocated = other.allocated;

        other.seek = 0;
        other.length = 0;
        other.buffer = nullptr;
        other.allocated_length = 0;
    }
    return *this;
}


IOStream& IOStream::operator=(const IOStream& other) noexcept {
    if (this != &other)
    {
        seek = other.seek;
        length = other.length;
        if(other.length > 0) {
            buffer = (char *) malloc(other.length);
            memcpy(buffer, other.buffer, other.length);
        }else{
            buffer = nullptr;
        }
        allocated_length = other.allocated_length;
        allocated = other.allocated;
    }
    return *this;
}


void IOStream::Flush()
{
    if(buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }

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
            buffer = (char *) realloc(buffer, allocated_length);

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
            buffer = (char *) realloc(buffer, allocated_length);

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

