#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <algorithm>
#include <iterator>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    RawMemory(size_t capacity)
            : buffer_(Allocate(capacity))
            , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
            : buffer_(other.buffer_)
            , capacity_(other.capacity_)
    {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }

        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
            : data_(size)
            , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
            : data_(other.size_)
            , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.Size(), data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
            : data_(std::move(other.data_))
            , size_(other.size_)
    {
        other.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (size_ > rhs.size_) {
                    std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
            rhs.size_ = 0;
        }
        return *this;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    void Swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        // constexpr оператор if будет вычислен во время компиляции
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_ + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_ + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_ + size_;
    }

    void Resize(size_t new_size);

    template<typename U>
    void PushBack(U &&value) {
        if (size_ == Capacity()) {
            Vector<T> tmp(0);
            tmp.Reserve(size_ == 0 ? 1 : size_ * 2);

            new (tmp.data_.GetAddress() + size_) T(std::forward<U>(value));
            tmp.size_ = size_ + 1;

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, tmp.data_.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, tmp.data_.GetAddress());
            }

            Swap(tmp);
        } else {
            new (data_.GetAddress() + size_) T(std::forward<U>(value));
            ++size_;
        }
    }

    void PopBack() /* noexcept */;

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            Vector<T> tmp(0);
            tmp.Reserve(size_ == 0 ? 1 : size_ * 2);

            new (tmp.data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            tmp.size_ = size_ + 1;

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, tmp.data_.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, tmp.data_.GetAddress());
            }

            Swap(tmp);
        } else {
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
        }

        return data_[size_ - 1];
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        const size_t offset = pos - data_.GetAddress();

        if (pos == cend()) {
            EmplaceBack(std::forward<Args>(args)...);
        } else if (size_ == Capacity()) {
            Vector<T> tmp(0);
            tmp.Reserve(size_ == 0 ? 1 : size_ * 2);

            new (tmp.data_.GetAddress() + offset) T(std::forward<Args>(args)...);
            tmp.size_ = size_ + 1;

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), offset, tmp.data_.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), offset, tmp.data_.GetAddress());
            }

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + offset, size_ - offset, tmp.data_.GetAddress() + offset + 1);
            } else {
                std::uninitialized_copy_n(data_.GetAddress() + offset, size_ - offset, tmp.data_.GetAddress() + offset + 1);
            }

            Swap(tmp);
        } else {
            T t(std::forward<Args>(args)...);
            std::uninitialized_move_n(data_.GetAddress() + size_, 1, data_.GetAddress() + size_);
            std::move_backward(data_.GetAddress() + offset, end() - 1, end());
            *(data_.GetAddress() + offset) = std::move(t);
            ++size_;
        }

        return begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& elem) {
        return Emplace(pos, elem);
    }

    iterator Insert(const_iterator pos, T&& elem) {
        return Emplace(pos, std::move(elem));
    }

    /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;
    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos <= end());
        const size_t offset = pos - data_.GetAddress();
        for (auto i = offset; i < size_ - 1; i++) {
            data_[i] = std::move(data_[i + 1]);
        }
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
        return begin() + offset;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

template<typename T>
void Vector<T>::Resize(size_t new_size) {
    Reserve(new_size);
    if (new_size < size_) {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    } else {
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
    }
    size_ = new_size;
}

template<typename T>
void Vector<T>::PopBack() {
    if (size_ > 0) {
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }
}
