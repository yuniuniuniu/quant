#include "XPluginEngine.hpp"
#include "XPluginTest.hpp"


int main(int argc, char* argv[])
{
    std::string soPlugin = "./libPluginTest.so";
    std::string errorString;
    PluginTest* object = XPluginEngine<PluginTest>::LoadPlugin(soPlugin, errorString);
    object->print();
    return 0;
}

// g++ -fPIC  ../XPluginEngineTest.cpp -o test -ldl