#ifndef TRANSPORTCATALOG_INTERVAL_MAP_H
#define TRANSPORTCATALOG_INTERVAL_MAP_H

#include <map>

template <typename K, typename V>
struct IntervalMap {
private:
    std::map<K, V> map;
public:
    void add(K const& keyBegin, K const& keyEnd, V const& val) {
        if (!(keyBegin < keyEnd)) {
            return;
        }

        auto endIter = map.lower_bound(keyEnd);
        if (endIter->first != keyEnd) {
            V const &oldVal = std::prev(endIter, 1)->second;
            endIter = map.insert(endIter, oldVal);
        }

        auto beginIter = map.lower_bound(keyBegin);
        if (beginIter->first != keyBegin) {
            beginIter = map.insert(beginIter, val);
            ++beginIter;
        }

        while(beginIter != endIter) {
            beginIter->second = val;
            ++beginIter;
        }
    }

    V const * operator[](K const& key) const {
        auto it = --map.upper_bound(key);
        if (!it)
            return nullptr;
        return &(it)->second;
    }
};

#endif //TRANSPORTCATALOG_INTERVAL_MAP_H
