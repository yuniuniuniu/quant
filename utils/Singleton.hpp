#ifndef SINGLETON_HPP
#define SINGLETON_HPP

namespace Utils
{
template <typename T>
class Singleton
{
public:
    static T *GetInstance()
    {
        return m_pInstance;
    }
private:
    static T *m_pInstance;
    Singleton() {}
};

template <typename T>
T *Singleton<T>::m_pInstance = new T;
}

#endif // SINGLETON_HPP
