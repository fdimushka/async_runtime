#include "ar/ar.hpp"


using namespace AsyncRuntime;


int main() {
    SetupRuntime();
    auto info = MakeNetAddrInfo("example.com");
    int ret = Await(AsyncNetAddrInfo(info));
    if(ret == IO_SUCCESS) {
        std::cout << info->hostname << std::endl;
    }else{
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
    }
    Terminate();
    return 0;
}
