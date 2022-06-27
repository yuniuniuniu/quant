#ifndef XPLUGINENGINE_HPP
#define XPLUGINENGINE_HPP
#include <dlfcn.h>
#include <string>

#define CreateObjectFunc(T)  extern "C" { T* CreateObject(){ return new T;}}


template <class T>
class XPluginEngine
{
public:
    static T* LoadPlugin(const std::string& soPlugin, std::string& errorString)
    {
        errorString.clear();
        T* ret = NULL;
        void* handler = dlopen(soPlugin.c_str(), RTLD_LAZY);
        if(NULL == handler)
        {
            errorString = dlerror();
        }
        else
        {
            typedef T* (*CreateObject)();
            CreateObject pFunc = (T* (*)())dlsym(handler, "CreateObject");
            if(NULL == pFunc)
            {
                errorString = dlerror();
            }
            else
            {
                ret = pFunc();
            }
            dlclose(handler);
        }
        return ret;
    }

};

#endif // XPLUGINENGINE_HPP