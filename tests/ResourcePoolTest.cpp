#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;
using namespace std;

TEST_CASE( "Create/delete resource", "[resource_pool]" ) {
    SetupRuntime();

    auto resource_id = AsyncRuntime::CreateResource();
    {
        auto coro = make_coroutine(resource_id, [=](coroutine_handler* handler, YieldVoid &yield) {
            auto resource = AsyncRuntime::GetResource(resource_id);
            REQUIRE(handler->get_resource() == resource);
        });


        Await(Async(coro));
    }
    AsyncRuntime::DeleteResource(resource_id);
    REQUIRE(AsyncRuntime::GetResource(resource_id) == nullptr);

    Terminate();
}
