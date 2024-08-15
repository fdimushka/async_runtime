#include "ar/gpu/gpu.hpp"
#include "ar/logger.hpp"

#include <opencv2/core/cuda.hpp>

using namespace AsyncRuntime;
using namespace AsyncRuntime::GPU;

void GPU::InitDevice() {
    int devices_count = cv::cuda::getCudaEnabledDeviceCount();
    int device_id = cv::cuda::getDevice();
    cv::cuda::DeviceInfo device_info(device_id);
    AR_LOG_SS(Info, "Used GPU device: " << device_info.name() << ", CUDA arch version:" << device_info.majorVersion() << "." << device_info.minorVersion());
}