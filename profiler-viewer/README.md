## Viewer for AR profiler

This is a real-time web viewer for your application's profiling data.
It preview this data:
* System info.
* Async functions work time tracking peer threads.
* Info of functions call.
* Info of threads.
* Coroutines count.

![profiler](../docs/profiler.png)


### Build and run instructions
To build you will need:
* node >= v16.15.0
* npm >= 8.5.5

Build:
```
cd profiler-viewer
npm install
npm run build
``` 

Run viewer web server:
```
npm run start
```
