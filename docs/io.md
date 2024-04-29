## Async I/O

AR supported asynchronous I/O. 
Asynchronous I/O based on [libuv](https://github.com/libuv/libuv) library. 
Full-featured event loop backed by epoll, kqueue, IOCP, event ports.

### Feature highlights
* Asynchronous file and file system operations.
* Asynchronous TCP and UDP sockets.
* Asynchronous DNS resolution.
* IPC with socket sharing, using Unix domain sockets or named pipes (Windows).

### API (async fs io)
* `class IOStream` - is primitive to implement a buffer for async reading and writing to fs.

* `IOFsStreamPtr MakeStream()` - method for create empty stream for reading. Return shared pointer to stream object.
* `IOFsStreamPtr MakeStream(const char *buffer, size_t length)` - method for create stream with data for writing. Return shared pointer to stream object.
* `IOResultPtr AsyncFsOpen(const IOFsStreamPtr& stream, const char* filename, int flags, int mode)` - method for async open file descriptor, `stream` - is shared pointer to file stream, `filename` - path to file, `flags`, `mode` - the file status flags and file access modes of the open file description shall be set according to the value of oflag ([see details - open(3)](https://linux.die.net/man/3/open)). Return shared pointer to promise result, if completed successfully resolve code - `int(0)` or error code, [see error details](http://docs.libuv.org/en/v1.x/errors.html), for parse error code use `const char* FSErrorMsg(int error)` or `const char* FSErrorName(int error)`.
* `IOResultPtr AsyncFsClose(const IOFsStreamPtr& stream)` - method for async close a file, `stream` - is shared pointer to file stream. Return shared pointer to promise result. Successfully result code is `0`. [Error codes detail](http://docs.libuv.org/en/v1.x/errors.html).
* `IOResultPtr AsyncFsRead(const IOFsStreamPtr& stream, int64_t seek = -1, int64_t size = -1)` - method for async read from file, `stream` - is shared pointer to file stream, `seek` - offset reading (by default without offset), `size` - number of bytes to read (by default read all data from file). The read data will be written to the stream buffer. Return shared pointer to promise result. Successfully result code is `0`. [Error codes detail](http://docs.libuv.org/en/v1.x/errors.html).
* `IOResultPtr AsyncFsWrite(const IOFsStreamPtr& stream, int64_t seek = -1)` - method for async write to file, `stream` - is shared pointer to file stream, `seek` - offset writing (by default without offset). Write data will be reading from the stream buffer. Return shared pointer to promise result. Successfully result code is `0`. [Error codes detail](http://docs.libuv.org/en/v1.x/errors.html).


### API (async network io)
#### Objects:
* `class IOStream` - is primitive to implement a buffer for async reading and writing to socket.
* `class TCPServer` - is primitive of server data struct, use `MakeTCPServer(const char* hostname, int port)` for make instance of `TCPServer`.
* `class TCPConnection` - tcp connection data struct, use `MakeTCPConnection(const char *hostname, int port, int keepalive)` for make instance of `TCPConnection`.
* `class TCPSession` - tcp session data struct.
* `class NetAddrInfo` - dns info data struct, use `MakeNetAddrInfo(const char* node)` for make instance of `NetAddrInfo`.

#### Methods:
* `IOResultPtr AsyncListen(const TCPServerPtr& server, const TCPSession::HandlerType & handle_connection)` - method for async listen connections.
* `IOResultPtr AsyncRead(const TCPSessionPtr & session, const IOStreamPtr & stream)`.
* `IOResultPtr AsyncRead(const TCPConnectionPtr & connection, const IOStreamPtr & stream)`.
* `IOResultPtr AsyncWrite(const TCPSessionPtr & session, const IOStreamPtr & stream)`.
* `IOResultPtr AsyncWrite(const TCPConnectionPtr & connection, const IOStreamPtr & stream)`.
* `IOResultPtr AsyncClose(const TCPSessionPtr & session)`.
* `IOResultPtr AsyncClose(const TCPConnectionPtr & connection)`.
* `IOResultPtr AsyncNetAddrInfo(const NetAddrInfoPtr & info)`.

### Usage example
Include library headers:
``` C++
#include <ar/ar.hpp>
using namespace AsyncRuntime;
```

Init default runtime with default setting
``` C++
...
SetupRuntime();
...
```
Create file stream:
```C++
auto in_stream = MakeStream();
```

Async open file, call it in async function:
```C++
int res = Await(AsyncFsOpen(in_stream, "path/to/file"), handler)); // handler - it is pointer to the coroutine object.
```

Async read from file:
```C++
int res = Await(AsyncFsRead(in_stream), handler));
if(res == IO_SUCCESS) {
    std::cout << in_stream->GetBuffer() << std::endl;
}else{
    std::cerr << "Error read file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
}
```

Async close file:
```C++
Await(AsyncFsClose(in_stream), handler);
```

### Examples

* Async tcp server example [see here](../examples/tcp_server.cpp).
* Async tcp client example [see here](../examples/http_client.cpp).
* Async dns example [see here](../examples/dns.cpp).
* Async fs example [see here](../examples/io.cpp).
