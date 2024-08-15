#include "ar/ar.hpp"
#include "ar/logger.hpp"
#include "ar/gpu/gpu.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudawarping.hpp>

using namespace AsyncRuntime;
using namespace std::chrono_literals;

void cv_cuda_stream_callback(int status, void* userData) {
    std::cout << "cuda stream callback: " << status << std::endl;
}

int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();
    GPU::InitDevice();
    cv::cuda::Stream stream;


    cv::Mat src(cv::Size(1920, 1080), CV_8UC3, cv::Scalar(0)), dst;

    cv::cuda::GpuMat gpu_src, gpu_dst;
    for(;;) {

        std::cout << "upload" << std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();
        stream.enqueueHostCallback(&cv_cuda_stream_callback, nullptr);
        gpu_src.upload(src, stream);


        std::cout << "resize" << std::endl;
        //stream.enqueueHostCallback(&cv_cuda_stream_callback, nullptr);
        cv::cuda::resize(gpu_src, gpu_dst, cv::Size(640, 640), 0, 0, cv::INTER_LINEAR, stream);

        std::cout << "download" << std::endl;
        //stream.enqueueHostCallback(&cv_cuda_stream_callback, nullptr);
        gpu_src.download(dst, stream);


        std::cout << "wait" << std::endl;

        stream.waitForCompletion();
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        std::cout << "success: " << elapsed_time_ms << std::endl;

    }
    Terminate();
    return 0;
}

