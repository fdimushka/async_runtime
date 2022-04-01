#include "ar/stream.hpp"
#include <algorithm>
#include <numeric>


using namespace AsyncRuntime;


IOFsStream::IOFsStream() : fd(-1), read_buf_len(0)
{

}


IOFsStream::~IOFsStream()
{
    Flush();
}


uv_buf_t *IOFsStream::CreateReadBuffer(size_t len)
{
    uv_buf_t buf;
    buf.base = (char*)malloc(sizeof(char*) * len);
    buf.len = len;
    read_bufs.push_back(buf);
    return ReadBuffer();
}


uv_buf_t *IOFsStream::ReadBuffer()
{
    assert(!read_bufs.empty());
    return &read_bufs.back();
}


void IOFsStream::IncreaseReadBufferLength(int64_t len)
{
    read_buf_len += len;
}


int64_t IOFsStream::GetReadBufferSize() const
{
    return read_buf_len;
}


void IOFsStream::Flush()
{
    for(auto & buf : read_bufs) {
        if(buf.base != nullptr) {
            free(buf.base);
            buf.base = nullptr;
        }
    }

    read_buf_len = 0;
    read_bufs.clear();
}

