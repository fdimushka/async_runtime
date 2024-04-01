#include "ar/opencv_executor.h"

using namespace AsyncRuntime;

OpenCVExecutor::OpenCVExecutor(const std::string &name) : IExecutor(name, kUSER_EXECUTOR) {
}

OpenCVExecutor::~OpenCVExecutor() {
}

void OpenCVExecutor::Post(Task *task) {

}
