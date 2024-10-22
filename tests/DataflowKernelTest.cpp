#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "ar/ar.hpp"
#include "ar/dataflow/dataflow.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::Dataflow;

class TestKernelContext : public KernelContext {
public:
    int data = 0;

    ~TestKernelContext() override = default;
};

TEST_CASE( "run/terminate kernel", "[kernel]" ) {
    SetupRuntime();
    SECTION( "init/run/terminate" ) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;
            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") { };

            ~kernel_test() override { }

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        REQUIRE(kernel->Run() == true);
        REQUIRE(Await(kernel->AsyncTerminate()) == 0);
    }

    SECTION( "init fail" ) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;
            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") { };

            ~kernel_test() override { }

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return -1; }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == -1);
        REQUIRE(kernel->Run() == false);
        Await(kernel->AsyncTerminate());
    }

    SECTION( "then test" ) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;
            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") { };

            ~kernel_test() override { }

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        bool terminated_call = false;
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(kernel->Run([&terminated_call](int error){
            terminated_call = true;
        })== true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Await(kernel->AsyncTerminate());
        REQUIRE(terminated_call == true);
    }

    SECTION( "double run" ) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;
            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") { };

            ~kernel_test() override { }

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        REQUIRE(kernel->Run() == true);
        REQUIRE(Await(kernel->AsyncTerminate()) == 0);
        REQUIRE(kernel->Run() == false);
    }

    SECTION( "destroy kernel after init" ) {
        class TestKernelContext2 : public KernelContext {
        public:
            int data = 0;

            ~TestKernelContext2() override {
                std::cout << "~TestKernelContext2" << std::endl;
            }
        };

        class kernel_test : public Kernel<TestKernelContext2> {
        public:
            typedef Dataflow::Kernel<TestKernelContext2> super;
            kernel_test() : Dataflow::Kernel<TestKernelContext2>("test_kernel") { };

            ~kernel_test() override {
                std::cout << "destroy kernel" << std::endl;
            }

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext2 *context) noexcept override { return 0; }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext2 *context) override {
                return Dataflow::KernelProcessResult::kNEXT;
            }
        };

        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
    }

    Terminate();
}

TEST_CASE( "run/terminate kernel with resource pool", "[kernel]" ) {
    SetupRuntime();

    SECTION( "init/run/terminate" ) {

        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;
            explicit kernel_test(resource_pool *res) : resource(res), Dataflow::Kernel<TestKernelContext>(res, "test_kernel") { };

            ~kernel_test() override = default;

            int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override {
                REQUIRE(handler->get_resource() == resource);
                return 0;
            }

            KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                REQUIRE(handler->get_resource() == resource);
                return Dataflow::KernelProcessResult::kEND;
            }
        private:
            resource_pool *resource;
        };

        auto resource_id = CreateResource();
        {
            auto resource = GetResource();
            std::shared_ptr<kernel_test> kernel = make_shared_ptr<kernel_test>(resource, resource);
            REQUIRE(Await(kernel->AsyncInit()) == 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            REQUIRE(kernel->Run() == true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            REQUIRE(Await(kernel->AsyncTerminate()) == 0);
        }
        DeleteResource(resource_id);
    }

    Terminate();
}

TEST_CASE( "run/terminate kernel in coroutine", "[kernel]" ) {
    SetupRuntime();
    auto coro = make_coroutine([](coroutine_handler *handler, yield<void> & yield) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;

            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") {};

            ~kernel_test() override { }

            int
            OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

            KernelProcessResult
            OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                sleep(2);
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        bool terminated_call = false;
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit(), handler) == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(kernel->Run([&terminated_call](int error){
                        terminated_call = true;
        }) == true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(Await(kernel->AsyncTerminate(), handler) == 0);
        REQUIRE(terminated_call == true);
    });

    Await(Async(coro));

    Terminate();
}

TEST_CASE( "init/terminate kernel in coroutine without run", "[kernel]" ) {
    SetupRuntime();
    auto coro = make_coroutine([](coroutine_handler *handler, yield<void> & yield) {
        class kernel_test : public Kernel<TestKernelContext> {
        public:
            typedef Dataflow::Kernel<TestKernelContext> super;

            kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") {};

            ~kernel_test() override { }

            int
            OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

            KernelProcessResult
            OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
                sleep(2);
                return Dataflow::KernelProcessResult::kEND;
            }
        };

        bool terminated_call = false;
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit(), handler) == 0);
        REQUIRE(Await(kernel->AsyncTerminate(), handler) == 0);
    });

    Await(Async(coro));

    Terminate();
}

TEST_CASE( "kernel sink subscription events", "[kernel]" ) {
    class kernel_test : public Kernel<TestKernelContext> {
    public:
        typedef Dataflow::Kernel<TestKernelContext> super;
        kernel_test() : Dataflow::Kernel<TestKernelContext>("test_kernel") {
            source.Add<int>("input");
            sink.Add<int>("output");
        };

        ~kernel_test() override { }

        int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

        KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override {
            return Dataflow::KernelProcessResult::kEND;
        }

        KernelProcessResult OnSinkSubscription(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) final {
            call_subscription++;
            return super::OnSinkSubscription(handler, context);
        }

        KernelProcessResult OnSinkUnsubscription(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) final {
            call_unsubscription++;
            return super::OnSinkUnsubscription(handler, context);
        }

        KernelProcessResult OnWaitSinkSubscription(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) final {
            call_wait_subscription.store(1);
            return super::OnWaitSinkSubscription(handler, context);
        }

        KernelProcessResult OnTerminate(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) final {
            call_terminate.store(1);
            return super::OnTerminate(handler, context);
        }

        std::atomic_int     call_terminate = {0};
        std::atomic_int     call_subscription = {0};
        std::atomic_int     call_unsubscription = {0};
        std::atomic_int     call_wait_subscription = {0};
    };


    SetupRuntime();


    SECTION( "Subscribe/Unsubscribe" ) {
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        REQUIRE(kernel->Run() == true);
        Dataflow::PortUser port_user;
        REQUIRE(kernel->GetSink().SubscribersEmpty() == true);
        kernel->GetSink().Subscribe("output", &port_user);
        REQUIRE(kernel->GetSink().SubscribersEmpty() == false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_subscription.load() >= 1);
        kernel->GetSink().Unsubscribe("output", &port_user);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_unsubscription.load() >= 1);
        REQUIRE(kernel->call_wait_subscription.load() >= 1);
        REQUIRE(kernel->GetSink().SubscribersEmpty() == true);
        Await(kernel->AsyncTerminate());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_terminate.load() >= 1);
    }

    SECTION( "Unsubscribe/Subscribe/Unsubscribe" ) {
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        REQUIRE(kernel->Run() == true);
        Dataflow::PortUser port_user;
        REQUIRE(kernel->GetSink().SubscribersEmpty() == true);

        kernel->GetSink().Unsubscribe("output", &port_user);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_unsubscription.load() == 0);

        kernel->GetSink().Subscribe("output", &port_user);
        REQUIRE(kernel->GetSink().SubscribersEmpty() == false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_subscription.load() >= 1);

        kernel->GetSink().Unsubscribe("output", &port_user);
        REQUIRE(kernel->GetSink().SubscribersEmpty() == true);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_unsubscription.load() >= 1);
        REQUIRE(kernel->call_wait_subscription.load() >= 1);

        Await(kernel->AsyncTerminate());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->call_terminate.load() >= 1);
    }

    SECTION( "Many subscribers" ) {
        std::shared_ptr<kernel_test> kernel = std::make_shared<kernel_test>();
        REQUIRE(Await(kernel->AsyncInit()) == 0);
        REQUIRE(kernel->Run() == true);
        Dataflow::PortUser port_users[10];
        REQUIRE(kernel->GetSink().SubscribersEmpty() == true);
        for( int i = 0; i < 10; ++i ) {
            kernel->GetSink().Subscribe("output", &port_users[i]);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel->GetSink().SubscribersEmpty() == false);
        REQUIRE(kernel->call_subscription.load() >= 10);
        Await(kernel->AsyncTerminate());
    }
}


