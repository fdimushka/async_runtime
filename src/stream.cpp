#include "ar/stream.hpp"
#include <algorithm>
#include <numeric>


using namespace AsyncRuntime;


IOStreamBuffer::IOStreamBuffer() : seek(0), length(0), allocated_length(0), allocated(false) { }
IOStreamBuffer::IOStreamBuffer(const char *buf, int64_t len) : seek(0), length(len), allocated_length(len), allocated(true)
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


IOStreamBuffer::~IOStreamBuffer()
{
    Flush();
}


uv_buf_t *IOStreamBuffer::Next(int64_t size)
{
    if(!allocated) {
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

        seek += size;
    }else{
        if(seek >= length)
            return nullptr;

        int64_t s = std::min(size, length - seek);
        uv_buf.base = buffer + seek;
        uv_buf.len = s;
        seek += s;
    }

    return &uv_buf;
}


void IOStreamBuffer::Flush()
{
    if(buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }

    allocated_length = 0;
    length = 0;
    seek = 0;
}


IOFsStream::IOFsStream() : fd(-1) { }
IOFsStream::IOFsStream(const char* buf, int64_t len) : fd(-1), write_stream(buf, len) { }


IOFsStream::~IOFsStream()
{
    Flush();
}


void IOFsStream::Flush()
{
    read_stream.Flush();
    write_stream.Flush();
}

