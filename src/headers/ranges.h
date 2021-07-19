#ifndef RANGES_H
#define RANGES_H

#include <unordered_set>

namespace Ranges {
    template <typename Iterator>
    struct Range {
    public:
        Range(Iterator first, Iterator last) : m_begin(first), m_end(last) {}

        Iterator begin() const { return m_begin;}
        Iterator end() const { return m_end;}

        std::reverse_iterator<Iterator> rbegin() const { return std::make_reverse_iterator(m_end);}
        std::reverse_iterator<Iterator> rend() const { return std::make_reverse_iterator(m_begin);}

    private:
        Iterator m_begin;
        Iterator m_end;
    };

    template <typename Container>
    Range<typename Container::const_iterator> AsRange(Container const & container) {
        return Range{std::begin(container), std::end(container)};
    }

    template <typename Iter>
    unsigned int GetUniqueItems(Range<Iter> range){
        return std::unordered_set<typename std::iterator_traits<Iter>::value_type>{range.begin(), range.end()}.size();
    }

    template <typename Iter>
    Range<std::reverse_iterator<Iter>> Reverse(Range<Iter> range){
        return Range{range.rbegin(), range.rend()};
    }

    template <typename Iter>
    Range<Iter> ToMiddle(Range<Iter> range){
        return Range{range.begin(), range.begin() + std::distance(range.begin(), range.end()) / 2 + 1};
    }

    template <typename Iter>
    Range<Iter> FromMiddle(Range<Iter> range){
        return Range{range.begin() + std::distance(range.begin(), range.end()) / 2, range.end()};
    }
}


#endif //RANGES_H
