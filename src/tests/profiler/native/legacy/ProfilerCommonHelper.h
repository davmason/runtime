// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef __PROFILER_COMMON_HELPER__
#define __PROFILER_COMMON_HELPER__

#include <cassert>
#include <cstring>
#include <algorithm>
#include <map>

template<class Key, class T, class Compare, class Allocator, template <class, class, class, class> class MapType>
void FreeAndEraseMap(MapType<Key, T, Compare, Allocator> &map)
{
    std::for_each(map.begin(), map.end(), [](std::pair<const Key, T> x) { delete x.second; });
    map.clear();
}

#endif //__PROFILER_COMMON_HELPER__
