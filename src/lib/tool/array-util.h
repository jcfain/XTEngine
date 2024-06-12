#ifndef ARRAYUTIL_H
#define ARRAYUTIL_H

#include <QMap>

class ArrayUtil {
public:
    /// Returns 0 if nothing is found, a pointer if found.
    template <typename K, typename V>
    static V* FindByValue(QMap<K, V> map, std::function<bool(const V& item)> predicate)
    {
        auto value = std::find_if(map.begin(), map.end(), predicate);
        if(value == map.end()) {
            return 0;
        }
        return value;
    }
};

#endif // ARRAYUTIL_H
