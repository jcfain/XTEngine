#ifndef ARRAYUTIL_H
#define ARRAYUTIL_H

#include <QMap>

class ArrayUtil {
public:
    /// Returns 0 if nothing is found, a pointer if found.
    // template <typename K, typename V>
    // static V* FindByValue(QMap<K, V> map, std::function<bool(const V& item)> predicate)
    // {
    //     auto value = std::find_if(map.begin(), map.end(), predicate);
    //     if(value != map.end())
    //         return &map[value - list.begin()];
    //     return 0;
    // }
    template <typename V>
    static V* FindByValue(QList<V> list, std::function<bool(const V& item)> predicate)
    {
        auto value = std::find_if(list.begin(), list.end(), predicate);
        if(value != list.end())
            return &list[value - list.begin()];
        return 0;
    }
};

#endif // ARRAYUTIL_H
