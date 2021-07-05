#ifndef __PROFILER_COMMON_HELPER__
#define __PROFILER_COMMON_HELPER__

#include <cassert>
#include <cstring>
#include <algorithm>


template<typename T>
void FreeAndEraseMap(T &map)
{
    std::for_each(map.begin(), map.end(), [](auto x) { delete x.second; });
    map.clear();
}

#endif //__PROFILER_COMMON_HELPER__