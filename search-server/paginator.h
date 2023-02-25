#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : range_begin_(begin)
        , range_end_(end)
        , range_size_(distance(begin, end)) {

    }

    Iterator begin() const {
        return range_begin_;
    }

    Iterator end() const {
        return range_end_;
    }

    size_t size() const {
        return range_size_;
    }

private:
    Iterator range_begin_;
    Iterator range_end_;
    size_t range_size_;
};

template <typename Iterator>
class Paginator {
    // тело класса
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = std::next(begin, current_page_size);
            pages_.push_back({ begin, current_page_end });

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        std::cout << (*it);
    }
    return os;
}