## Runtime profiling and tracing tool

Inner profilers allows you to get the following information:
* System info.
* Async functions work time tracking peer threads.
* Info of functions call.
* Info of threads.
* Coroutines count.


### Build with profiling.
For build library with profiler use `cmake` option `-DWITH_PROFILER=ON`.
For example:
```
cmake -DWITH_PROFILER=ON ...
```
Profiler use [flatbuffers](https://google.github.io/flatbuffers/) for share his state. 
Fbs scheme you can see here: [flatbuffers profiling scheme](../src/fbs/profiler_schema.fbs).
Build system will compile and gen c++ header and ts scripts automatic in compile time.

### How to use profiler
Include this header: `ar/ar.hpp`
For adding async function in profiling use this macros: `PROFILER_REG_ASYNC_FUNCTION(ID)` where `ID` is unique function, for example `CoroutineHandler` id. 
Example:
```c++
...
void async_fun_a(CoroutineHandler* handler, YieldVoid & yield) {
    PROFILER_REG_ASYNC_FUNCTION(handler->GetID());
    yield();
    ...
}
...
```

For configure profiler user this commands before `SetupRuntime()`:
* `PROFILER_SET_APP_INFO(argc, argv)` - set app info.
* `PROFILER_SET_PROFILING_INTERVAL(5min)` - set time profiling interval (by example: 5min).
* `PROFILER_SET_SERVER_HOST("0.0.0.0")` - set profiler server host.
* `PROFILER_SET_SERVER_PORT(9002)` - set profiler server port.

More [examples of using](../examples/profiler.cpp).

### How to see profiling result
[docs of profiling web viewer](../profiler-viewer/README.md).
