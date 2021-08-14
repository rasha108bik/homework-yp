#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <string>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t new_capacity) {
        capacity_ = new_capacity;
    }
    size_t GetCapacity() const {
        return capacity_;
    }

private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    explicit SimpleVector() noexcept = default;

    explicit SimpleVector(ReserveProxyObj obj) {
        Reserve(obj.GetCapacity());
    };

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, Type{}) {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, Type value) :
                data_(size),
                size_(size),
                capacity_(size) {
        for (size_t i = 0; i < size; ++i) {
            data_[i] = std::move(value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) :
                data_(init.size()),
                size_(static_cast<size_t>(init.size())),
                capacity_(static_cast<size_t>(init.size())) {
        std::copy(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()), begin());
    }

    SimpleVector(SimpleVector&& other) {
        SimpleVector tmp(other.size_);
        std::copy(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), tmp.begin());
        swap(tmp);
        other.size_ = 0u;
    }

    SimpleVector(const SimpleVector& other) {
        SimpleVector tmp(other.size_);
        std::copy(other.begin(), other.end(), tmp.data_.Get());
        swap(tmp);
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            swap(rhs);
            rhs.size_ = 0;
        }
        return *this;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (rhs.IsEmpty()) {
            Clear();
        }
        if (this != &rhs) {
            SimpleVector tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type&& item) {
        if (size_ == 0u) {
            Resize(1);
            size_ = 0u;
        } else if (size_ == capacity_) {
            auto old_size = size_;
            Resize(2 * size_);
            size_ = old_size;
        }
        data_[size_++] = std::move(item);
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        auto r_pos = const_cast<Type*>(pos);

        if (size_ < capacity_){
            std::copy_backward(std::make_move_iterator(r_pos), std::make_move_iterator(end()), end() + 1);
            *r_pos = std::move(value);
            ++size_;

            r_pos = begin() + std::distance(begin(), r_pos);
        } else if (size_ >= capacity_ && size_ != 0u) {
            SimpleVector<Type> tmp(2 * size_);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.begin());

            auto dist = std::distance(begin(), r_pos);
            std::copy_backward(std::make_move_iterator(tmp.begin() + dist), std::make_move_iterator(tmp.end()), tmp.begin() + tmp.size_+ 1);
            tmp[dist] = std::move(value);

            ++size_;
            capacity_ = 2 * size_;
            data_.swap(tmp.data_);

            r_pos = begin() + dist;
        } else if (size_ >= capacity_ && size_ == 0u) {
            SimpleVector<Type> tmp(size_ + 1);
            tmp[size_] = std::move(value);
            swap(tmp);

            r_pos = begin();
        }

        return r_pos;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        auto r_pos = const_cast<Type*>(pos);

        if (size_ < capacity_){
            std::copy_backward(std::make_move_iterator(r_pos), std::make_move_iterator(end()), end() + 1);
            *r_pos = std::move(value);
            ++size_;

            r_pos = begin() + std::distance(begin(), r_pos);
        } else if (size_ >= capacity_ && size_ != 0u) {
            SimpleVector<Type> tmp(2 * size_);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.begin());

            auto dist = std::distance(begin(), r_pos);
            std::copy_backward(std::make_move_iterator(tmp.begin() + dist), std::make_move_iterator(tmp.end()), tmp.begin() + tmp.size_+ 1);
            tmp[dist] = std::move(value);

            ++size_;
            capacity_ = 2 * size_;
            data_.swap(tmp.data_);

            r_pos = begin() + dist;
        } else if (size_ >= capacity_ && size_ == 0u) {
            SimpleVector<Type> tmp(size_ + 1);
            tmp[size_] = std::move(value);
            swap(tmp);

            r_pos = begin();
        }

        return r_pos;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (!IsEmpty()) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end());
        auto r_pos = const_cast<Type*>(pos);

        SimpleVector<Type> tmp_back(size_);
        std::copy_backward(std::make_move_iterator(r_pos + 1), std::make_move_iterator(end()), tmp_back.end());

        SimpleVector<Type> tmp_front(2 * size_);
        std::copy(std::make_move_iterator(begin()), std::make_move_iterator(r_pos), tmp_front.begin());

        auto dist = std::distance(begin(), r_pos);
        std::copy(std::make_move_iterator(tmp_back.begin() + dist + 1), std::make_move_iterator(tmp_back.end()), tmp_front.begin() + dist);

        --size_;
        data_.swap(tmp_front.data_);

        r_pos = begin() + dist;
        return r_pos;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0u;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        using namespace std::string_literals;

        if (index >= size_) {
            throw std::out_of_range("out of range"s);
        }
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        using namespace std::string_literals;

        if (index >= size_) {
            throw std::out_of_range("out of range"s);
        }
        return data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0u;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            auto new_capacity = 2 * new_size;
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.Get());
            data_.swap(tmp);
            capacity_ = new_capacity;
        } else if (new_size > size_ && new_size < capacity_) {
            ArrayPtr<Type> tmp(new_size);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.Get());
            data_.swap(tmp);
        } else if (new_size < size_) {
            ArrayPtr<Type> tmp(new_size);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(begin() + new_size), tmp.Get());
            data_.swap(tmp);
        }

        size_ = new_size;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.Get());
            data_.swap(tmp);
            capacity_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return data_.Get() + size_;
    }

private:
    ArrayPtr<Type> data_;

    size_t size_ = 0u;
    size_t capacity_ = 0u;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs < rhs) || (lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs) || (lhs == rhs);
}