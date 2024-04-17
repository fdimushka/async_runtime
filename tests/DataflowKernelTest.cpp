#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "ar/ar.hpp"
#include "ar/dataflow/dataflow.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::Dataflow;

struct TestKernelContext : KernelContext {
    int data = 0;
};

class TestKernel : public Kernel<TestKernelContext> {
public:
    typedef Dataflow::Kernel<TestKernelContext> super;
    TestKernel() : Dataflow::Kernel<TestKernelContext>("test_kernel")
    {
        source.Add<int>("input");
        sink.Add<int>("output");
    };

    ~TestKernel() override
    {
        Terminate();
    }

    int OnInit(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) noexcept override { return 0; }

    KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, TestKernelContext *context) override
    {
        return Dataflow::KernelProcessResult::kNEXT;
    }
};

class TestSinkSubscriptionKernel : public TestKernel {
public:
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

TEST_CASE( "run/terminate kernel", "[kernel]" ) {
    SetupRuntime();
    TestKernel kernel;
    kernel.Run();
    kernel.Terminate();
}

TEST_CASE( "kernel sink subscription events", "[kernel]" ) {
    SetupRuntime();


    SECTION( "Subscribe/Unsubscribe" ) {
        TestSinkSubscriptionKernel kernel;
        kernel.Run();
        Dataflow::PortUser port_user;
        REQUIRE(kernel.GetSink().SubscribersEmpty() == true);
        kernel.GetSink().Subscribe("output", &port_user);
        REQUIRE(kernel.GetSink().SubscribersEmpty() == false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_subscription.load() == 1);
        kernel.GetSink().Unsubscribe("output", &port_user);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_unsubscription.load() == 1);
        REQUIRE(kernel.call_wait_subscription.load() == 1);
        REQUIRE(kernel.GetSink().SubscribersEmpty() == true);
        kernel.Terminate();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_terminate.load() == 1);
    }

    SECTION( "Unsubscribe/Subscribe/Unsubscribe" ) {
        TestSinkSubscriptionKernel kernel;
        kernel.Run();
        Dataflow::PortUser port_user;
        REQUIRE(kernel.GetSink().SubscribersEmpty() == true);

        kernel.GetSink().Unsubscribe("output", &port_user);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_unsubscription.load() == 0);
        REQUIRE(kernel.call_wait_subscription.load() == 0);

        kernel.GetSink().Subscribe("output", &port_user);
        REQUIRE(kernel.GetSink().SubscribersEmpty() == false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_subscription.load() == 1);

        kernel.GetSink().Unsubscribe("output", &port_user);
        REQUIRE(kernel.GetSink().SubscribersEmpty() == true);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_unsubscription.load() == 1);
        REQUIRE(kernel.call_wait_subscription.load() == 1);

        kernel.Terminate();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.call_terminate.load() == 1);
    }

    SECTION( "Many subscribers" ) {
        TestSinkSubscriptionKernel kernel;
        kernel.Run();
        Dataflow::PortUser port_users[10];
        REQUIRE(kernel.GetSink().SubscribersEmpty() == true);
        for( int i = 0; i < 10; ++i ) {
            kernel.GetSink().Subscribe("output", &port_users[i]);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(kernel.GetSink().SubscribersEmpty() == false);
        REQUIRE(kernel.call_subscription.load() == 10);
    }
}


