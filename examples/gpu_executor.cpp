#include "ar/ar.hpp"


namespace AsyncRuntime {
    typedef int GPUResult;
    typedef std::shared_ptr<Result<GPUResult>> GPUResultPtr;


    class GPUComputeTask : public Task {
    public:
        typedef GPUResult return_type;

        explicit GPUComputeTask() : result(new Result<return_type>()) {}

        void Execute(const ExecutorState &executor_) override { throw std::runtime_error("not implemented!"); }

        void Execute() {
            std::cout << "Compute GPU task" << std::endl;
            result->SetValue(0);
        };

        void Resolve(return_type res) { result->SetValue(res); }

        std::shared_ptr<Result<return_type>> GetResult() { return result; }
    private:
        std::shared_ptr<Result<return_type>> result;
    };


    class GPUExecutor  : public IExecutor {
    public:
        explicit GPUExecutor(std::string  name_)
        {
            gpu_thread = std::move(std::thread([this]() {
                while (true) {
                    auto v = queue.steal();
                    if (v) {
                        auto *task = v.value();
                        task->Execute();
                        delete task;
                    }
                }
            }));
        };

        ~GPUExecutor() override {
            gpu_thread.join();
        };

        GPUExecutor(const Executor&) = delete;
        GPUExecutor(Executor&&) = delete;

        GPUExecutor& operator =(const GPUExecutor&) = delete;
        GPUExecutor& operator =(GPUExecutor&&) = delete;

        void Post(Task* task) override { }

        void Post(GPUComputeTask* task) {
            queue.push(task);
        }
    private:
        WorkStealQueue<GPUComputeTask*> queue;
        std::thread gpu_thread;
    };


    inline GPUResultPtr AsyncGPUCompute() {
        return Runtime::g_runtime.AsyncTask<GPUExecutor, GPUComputeTask>();
    }
}


using namespace AsyncRuntime;


int main() {
    CreateExecutor<GPUExecutor>("GPUExecutor");
    SetupRuntime();

    GPUResult res = Await(AsyncGPUCompute());

    std::cout << "result from GPU: " << res << std::endl;

    return 0;
}

