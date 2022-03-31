## Async runtime library

[![C/C++ CI](https://github.com/fdimushka/async_runtime/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/fdimushka/async_runtime/actions/workflows/build.yml)

AR - it is a C++ library for efficient execution asynchronous tasks, io network operations and parallel heterogeneous computing. 

#### Threads & coroutines & scheduler
Runtime uses efficient work-stealing scheduler to optimize your multithreaded performance and trying to reduce CPU IO wait.
Used asymmetric coroutines based on context switcher implemented in [Boost.Context](https://github.com/boostorg/context).

[Learn more...](docs/arch.md)


## Documentation
* [Usage examples](docs/usage.md)
* [Reference section - all the details](docs/readme.md)


## Build instructions
To build you will need:

* cmake >= 3.12.4
* gcc >= 4.6
* boost >= 1.70.0
* libuv >= 1.44.1
* doxygen >= 1.9.3

```
mkdir build
cd cmake ..
make 
make install
``` 

CMake option:

| Option            | Description                       | Default |
| :---              |    :----:                         | :----:  |
| `WITH_TRACE`      | build with trace profiler         | OFF     |
| `WITH_TESTS`      | build with unittests              | OFF     |
| `WITH_EXAMPLES`   | build with samples                | ON      |
| `WITH_DOCS`       | build with docs                   | OFF      |