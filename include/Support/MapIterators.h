/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef SUPPORT_MAPITERATOR_H
#define SUPPORT_MAPITERATOR_H

#include <iterator>
#include <type_traits>
#include <utility>

template<bool Condition, class T>
struct add_const_if {
    typedef T type;
};

template<class T>
struct add_const_if<true, T> {
    typedef typename std::add_const<T>::type type;
};

template<class IteratorTy>
class key_iterator: public IteratorTy {
public:
    typedef typename add_const_if<
            std::is_const<
                    typename std::remove_reference<
                            typename std::iterator_traits<IteratorTy>::reference>::type>::value,
            typename std::iterator_traits<IteratorTy>::value_type::first_type>::type value_type;

    key_iterator() :
            IteratorTy() {
    }

    key_iterator(IteratorTy It) :
            IteratorTy(It) {
    }

    value_type& operator*() const {
        const IteratorTy& It = *this;
        return It->first;
    }

    value_type* operator->() const {
        const IteratorTy& It = *this;
        return &It->first;
    }
};

template<class IteratorTy>
class value_iterator: public IteratorTy {
public:
    typedef typename add_const_if<
            std::is_const<
                    typename std::remove_reference<
                            typename std::iterator_traits<IteratorTy>::reference>::type>::value,
            typename std::iterator_traits<IteratorTy>::value_type::second_type>::type value_type;

    value_iterator() :
            IteratorTy() {
    }

    value_iterator(IteratorTy It) :
            IteratorTy(It) {
    }

    value_type& operator*() const {
        const IteratorTy& It = *this;
        return It->second;
    }

    value_type* operator->() const {
        const IteratorTy& It = *this;
        return &It->second;
    }
};

template<class IteratorTy>
class KeyRange {
public:
    typedef key_iterator<IteratorTy> iterator;

private:
    iterator Begin;
    iterator End;

public:
    KeyRange(iterator B, iterator E) :
            Begin(B), End(E) {
    }

    iterator begin() const {
        return Begin;
    }

    iterator end() const {
        return End;
    }
};

template<class IteratorTy>
class ValueRange {
public:
    typedef value_iterator<IteratorTy> iterator;

private:
    iterator Begin;
    iterator End;

public:
    ValueRange(iterator B, iterator E) :
            Begin(B), End(E) {
    }

    iterator begin() const {
        return Begin;
    }

    iterator end() const {
        return End;
    }
};

template<class MapTy>
auto keys(MapTy&& Map) -> KeyRange<decltype(Map.begin())> {
    return KeyRange<decltype(Map.begin())>(Map.begin(), Map.end());
}

template<class MapTy>
auto values(MapTy&& Map) -> ValueRange<decltype(Map.begin())> {
    return ValueRange<decltype(Map.begin())>(Map.begin(), Map.end());
}

#endif