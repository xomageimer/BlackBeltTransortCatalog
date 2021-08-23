#ifndef TRANSPORTCATALOG_INTERVAL_MAP_H
#define TRANSPORTCATALOG_INTERVAL_MAP_H

#include <optional>
#include <map>

template <typename K, typename V>
struct IntervalMap {
private:
    std::map<K, std::optional<V>> m_map;
public:
    void add(K const& keyBegin, K const& keyEnd, V const& val) {
        if (keyBegin >= keyEnd)
            return;

        auto end_it = m_map.lower_bound(keyEnd);
        if (end_it == m_map.end() || end_it->first != keyEnd){
            if (end_it == m_map.begin()) {
                end_it = m_map.insert(end_it, {keyEnd, std::nullopt});
            } else {
                const auto & oldVal = std::prev(end_it, 1)->second;
                end_it = m_map.insert(end_it, {keyEnd, oldVal});
            }
        }

        auto beg_it = m_map.lower_bound(keyBegin);
        if (beg_it == m_map.end() || beg_it->first != keyBegin) {
            std::optional<V> val_opt;
            val_opt.emplace(val);
            beg_it = m_map.insert(beg_it, {keyBegin, std::move(val_opt)});
        }

        while(beg_it != end_it) {
            beg_it->second.emplace(val);
            ++beg_it;
        }
    }

    const std::optional<V> & operator[](K const& key) const {
        auto it = m_map.upper_bound(key);
        if (it == m_map.begin() || it != std::prev(m_map.end()) && std::next(it)->second == std::nullopt){
            return it->second;
        } else {
            if ((std::prev(it))->second == std::nullopt) {
                return std::prev((std::prev(it)))->second;
            } else {
                return (std::prev(it))->second;
            }
        }
    }
};

#endif //TRANSPORTCATALOG_INTERVAL_MAP_H
