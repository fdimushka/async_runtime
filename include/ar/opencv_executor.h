#ifndef AR_OPENCV_EXECUTOR_H
#define AR_OPENCV_EXECUTOR_H

#include "ar/executor.hpp"
#include "ar/io_task.hpp"
#include "uv.h"


namespace AsyncRuntime {

    /**
     * @brief
     * @class OpenCVExecutor
     */
    class OpenCVExecutor : public IExecutor {
    public:
        explicit OpenCVExecutor(const std::string & name);
        ~OpenCVExecutor() override;


        OpenCVExecutor(const OpenCVExecutor&) = delete;
        OpenCVExecutor(OpenCVExecutor&&) = delete;
        OpenCVExecutor& operator =(const OpenCVExecutor&) = delete;
        OpenCVExecutor& operator =(OpenCVExecutor&&) = delete;


        /**
         * @brief
         * @param task
         */
        void Post(Task* task) override;

    private:
    };
}


#endif //AR_OPENCV_EXECUTOR_H
