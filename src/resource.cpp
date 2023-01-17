#include "ar/resource.hpp"


using namespace AsyncRuntime;


Resource::Resource(const std::string &name)
{
    std::hash<std::string> hash;
    id = hash(name);
}
