#ifndef __PROFILER_COMMON_HELPER__
#define __PROFILER_COMMON_HELPER__

#include <cassert>
#include <cstring>
#include <algorithm>

template <class MetaInterface>
class COMPtrHolder
{
public:
    COMPtrHolder()
    {
        m_ptr = NULL;
    }
    
    COMPtrHolder(MetaInterface* ptr)
    {
        if (ptr != NULL)
        {
            ptr->AddRef();
        }
        m_ptr = ptr;
    }
    
    ~COMPtrHolder()
    {
        if (m_ptr != NULL)
        {
            m_ptr->Release();
            m_ptr = NULL;
        }
    }
    MetaInterface* operator->()
    {
        return m_ptr;
    }

    MetaInterface** operator&()
    {
       // _ASSERT(m_ptr == NULL);
        return &m_ptr;
    }
    
    operator MetaInterface*()
    {
        return m_ptr;
    }
private:
    MetaInterface* m_ptr;
};

template <class T>
class NewArrayHolder
{
public:
    NewArrayHolder()
    {
        m_pArray = NULL;
    }

    NewArrayHolder(T* pArray)
    {
        m_pArray = pArray;
    }

    NewArrayHolder<T>& operator=(T* other)
    {
        delete[] m_pArray;
        m_pArray = other;
        return *this;
    }

    operator T*()
    {
        return m_pArray;
    }

    ~NewArrayHolder()
    {
        delete[] m_pArray;
    }
private:
    NewArrayHolder(const NewArrayHolder& other)
    {
        assert(!"Unreachable");
    }

    T* m_pArray;
};

template<typename T>
void FreeAndEraseMap(T &map)
{
    std::for_each(map.begin(), map.end(), [](auto x) { delete x.second; });
    map.clear();
}

#endif //__PROFILER_COMMON_HELPER__