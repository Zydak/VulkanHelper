#pragma once

#include "Log/Log.h"
#include "Move.h"
#include <initializer_list>

namespace VulkanHelper
{
    /**
     * @brief A dynamically sized array similar to std::vector.
     *
     * Provides basic dynamic array functionality, including resizing, reserving capacity,
     * pushing and emplacing elements, and element access. Memory is managed manually using malloc/free.
     *
     * @note It's made instead of using std::vector to avoid ABI issues with different C++ standard library implementations.
     * Since I want this to work on multiple platforms, I don't want to rely on the standard library implementation.
     * And apparently, that's a really good practice when making a library anyway, it's mentioned almost everywhere, so who am I to argue?
     *
     * @tparam T The type of elements stored in the vector.
     */
    template<typename T>
    class Vector
    {
    public:
        /**
         * @brief Constructs an empty vector.
         *
         * Initializes the vector with zero size and capacity.
         */
        Vector()
            : m_Data(nullptr), m_Size(0), m_Capacity(0)
        {}

        /**
         * @brief Constructs a vector with a given initial size.
         *
         * Allocates memory for the specified number of elements and default-constructs them.
         * @param initialSize The initial number of elements in the vector.
         */
        explicit Vector(size_t initialSize)
            : m_Data(nullptr), m_Size(0), m_Capacity(0)
        {
            if (initialSize > 0)
            {
                Resize(initialSize);
            }
        }

        /**
         * @brief Constructs a vector with a given initial size and value.
         *
         * Allocates memory for the specified number of elements and copy-constructs them from the given value.
         * @param initialSize The initial number of elements in the vector.
         * @param value The value to copy-construct each element with.
         */
        explicit Vector(size_t initialSize, const T& value)
            : m_Data(nullptr), m_Size(0), m_Capacity(0)
        {
            if (initialSize > 0)
            {
                Resize(initialSize, value);
            }
        }

        /**
         * @brief Constructs a vector from an initializer list.
         *
         * @param initList The initializer list containing values to initialize the vector with.
         */
        Vector(std::initializer_list<T> initList)
            : m_Data(nullptr), m_Size(0), m_Capacity(0)
        {
            if (initList.size() > 0)
            {
                Reserve(initList.size());
                for (const auto& item : initList)
                {
                    PushBack(item);
                }
            }
        }

        /**
         * @brief Destructor. Destroys all elements and frees memory.
         */
        ~Vector()
        {
            Clear();
            free(m_Data);
            m_Data = nullptr;
            m_Size = 0;
            m_Capacity = 0;
        }

        /**
         * @brief Copy constructor. Creates a deep copy of another vector.
         * @param other The vector to copy from.
         */
        Vector(const Vector& other)
            : m_Data(nullptr), m_Size(0), m_Capacity(0)
        {
            if (other.m_Size > 0)
            {
                ChangeCapacity(other.m_Capacity);
                m_Size = other.m_Size;
                for (size_t i = 0; i < m_Size; i++)
                {
                    ConstructAt(i, other[i]);
                }
            }
        }

        /**
         * @brief Copy assignment operator. Replaces the contents with a copy of another vector.
         * @param other The vector to copy from.
         * @return Reference to this vector.
         */
        Vector& operator=(const Vector& other)
        {
            if (this == &other)
                return *this;

            Clear();
            free(m_Data);
            m_Data = nullptr;
            m_Size = 0;
            m_Capacity = 0;

            if (other.m_Size > 0)
            {
                ChangeCapacity(other.m_Capacity);
                m_Size = other.m_Size;
                for (size_t i = 0; i < m_Size; i++)
                {
                    ConstructAt(i, other[i]);
                }
            }

            return *this;
        }

        /**
         * @brief Move constructor. Takes ownership of another vector's data.
         * @param other The vector to move from.
         */
        Vector(Vector&& other) noexcept
            : m_Data(other.m_Data), m_Size(other.m_Size), m_Capacity(other.m_Capacity)
        {
            other.m_Data = nullptr;
            other.m_Size = 0;
            other.m_Capacity = 0;
        }

        /**
         * @brief Move assignment operator. Takes ownership of another vector's data.
         * @param other The vector to move from.
         * @return Reference to this vector.
         */
        Vector& operator=(Vector&& other) noexcept
        {
            if (this == &other)
                return *this;

            this->~Vector(); // Clean up current state

            m_Data = other.m_Data;
            m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;

            other.m_Data = nullptr;
            other.m_Size = 0;
            other.m_Capacity = 0;

            return *this;
        }

        /**
         * @brief Resizes the vector to the specified size.
         *
         * If the new size is greater than the current size, new elements are constructed (optionally with a value).
         * If the new size is less, excess elements are destroyed.
         * @tparam Args Optional argument for value-initialization.
         * @param newSize The new size of the vector.
         * @param args Optional value to initialize new elements with.
         */
        template<typename ... Args>
        void Resize(size_t newSize, Args&&... args)
        {
            static_assert(sizeof...(Args) <= 1, "Resize must be called with at most one argument");
            if (newSize > m_Capacity)
                ChangeCapacity(newSize);

            // Create new objects
            if (newSize > m_Size)
            {
                for (size_t i = m_Size; i < newSize; i++)
                {
                    ConstructAt(i, args...);
                }
            }

            if (newSize < m_Size)
            {
                // Explicitly call destructor for each element that is being removed
                for (size_t i = newSize; i < m_Size; i++)
                {
                    ((T*)m_Data)[i].~T();
                }
            }

            m_Size = newSize;
        }

        /**
         * @brief Ensures the vector has at least the specified capacity.
         *
         * If the requested capacity is greater than the current capacity, the buffer is reallocated.
         *
         * @param newCapacity The minimum capacity to reserve.
         */
        void Reserve(size_t newCapacity)
        {
            ChangeCapacity(newCapacity);
        }

        /**
         * @brief Copies the given value to the end of the vector.
         *
         * If the current size equals the capacity, the capacity is doubled.
         *
         * @param value The value to append (copied).
         */
        void PushBack(const T& value)
        {
            if (m_Size >= m_Capacity)
                ChangeCapacity(m_Capacity == 0 ? 1 : m_Capacity * GrowFactor);

            ConstructAt(m_Size, value);
            m_Size++;
        }

        /**
         * @brief Appends the given value to the end of the vector.
         *
         * If the current size equals the capacity, the capacity is doubled.
         *
         * @param value The value to append. (moved)
         */
        void PushBack(T&& value)
        {
            if (m_Size >= m_Capacity)
                ChangeCapacity(m_Capacity == 0 ? 1 : m_Capacity * GrowFactor);

            ConstructAt(m_Size, std::move(value));
            m_Size++;
        }

        /**
         * @brief Constructs a new element in-place at the end of the vector.
         *
         * If the current size equals the capacity, the capacity is doubled.
         *
         * @tparam Args Types of arguments to forward to the constructor of T.
         * @param args Arguments to forward to the constructor of T.
         */
        template<typename... Args>
        void EmplaceBack(Args&&... args)
        {
            if (m_Size >= m_Capacity)
                ChangeCapacity(m_Capacity == 0 ? 1 : m_Capacity * GrowFactor);
            ConstructAt(m_Size, std::forward<Args>(args)...);
            m_Size++;
        }

        /**
         * @brief Removes all elements from the vector, calling their destructors.
         *
         * The vector's size is set to zero, but capacity is unchanged.
         */
        void Clear()
        {
            for (size_t i = 0; i < m_Size; i++)
            {
                ((T*)m_Data)[i].~T(); // Explicitly call destructor for each element
            }
            m_Size = 0;
        }

        /**
         * @brief Checks if the vector is empty.
         * @return True if the vector contains no elements, false otherwise.
         */
        [[nodiscard]] bool Empty() const
        {
            return m_Size == 0;
        }

        /**
         * @brief Returns the number of elements in the vector.
         *
         * @return The current size of the vector.
         */
        [[nodiscard]] size_t Size() const
        {
            return m_Size;
        }

        /**
         * @brief Returns the current capacity of the vector.
         *
         * @return The number of elements that can be stored without reallocating.
         */
        [[nodiscard]] size_t Capacity() const
        {
            return m_Capacity;
        }

        /**
         * @brief Accesses the element at the specified index.
         *
         * @param index The index of the element to access.
         * @return Reference to the element at the given index.
         * @throws If index is out of range, triggers VH_ASSERT.
         */
        [[nodiscard]] T& operator[](size_t index)
        {
            VH_ASSERT(index < m_Size, "Index out of range");
            return ((T*)m_Data)[index];
        }

        /**
         * @brief Accesses the element at the specified index (const overload).
         *
         * @param index The index of the element to access.
         * @return Const reference to the element at the given index.
         * @throws If index is out of range, triggers VH_ASSERT.
         */
        [[nodiscard]] const T& operator[](size_t index) const
        {
            VH_ASSERT(index < m_Size, "Index out of range");
            return ((const T*)m_Data)[index];
        }

        /**
         * @brief Returns a pointer to the underlying data buffer.
         *
         * @return Pointer to the first element in the buffer, or nullptr if empty.
         */
        [[nodiscard]] T* Data()
        {
            return (T*)m_Data;
        }

        /**
         * @brief Returns a const pointer to the underlying data buffer.
         *
         * @return Const pointer to the first element in the buffer, or nullptr if empty.
         */
        [[nodiscard]] const T* Data() const
        {
            return (const T*)m_Data;
        }

        /**
         * @brief Returns a reference to the first element.
         *
         * @return Reference to the first element.
         * @throws If vector is empty, triggers VH_ASSERT.
         */
        [[nodiscard]] T& Front()
        {
            VH_ASSERT(m_Size > 0, "Vector is empty");
            return ((T*)m_Data)[0];
        }

        /**
         * @brief Returns a const reference to the first element.
         *
         * @return Const reference to the first element.
         * @throws If vector is empty, triggers VH_ASSERT.
         */
        [[nodiscard]] const T& Front() const
        {
            VH_ASSERT(m_Size > 0, "Vector is empty");
            return ((const T*)m_Data)[0];
        }

        /**
         * @brief Returns a reference to the last element.
         *
         * @return Reference to the last element.
         * @throws If vector is empty, triggers VH_ASSERT.
         */
        [[nodiscard]] T& Back()
        {
            VH_ASSERT(m_Size > 0, "Vector is empty");
            return ((T*)m_Data)[m_Size - 1];
        }

        /**
         * @brief Returns a const reference to the last element.
         *
         * @return Const reference to the last element.
         * @throws If vector is empty, triggers VH_ASSERT.
         */
        [[nodiscard]] const T& Back() const
        {
            VH_ASSERT(m_Size > 0, "Vector is empty");
            return ((const T*)m_Data)[m_Size - 1];
        }

        /**
         * @brief Removes the last element from the vector.
         *
         * @throws If vector is empty, triggers VH_ASSERT.
         */
        void PopBack()
        {
            VH_ASSERT(m_Size > 0, "Vector is empty");
            m_Size--;
            ((T*)m_Data)[m_Size].~T();
        }

        /**
         * @brief Returns an iterator to the first element.
         *
         * @return Iterator to the beginning of the vector.
         */
        [[nodiscard]] T* begin()
        {
            return (T*)m_Data;
        }

        /**
         * @brief Returns a const iterator to the first element.
         *
         * @return Const iterator to the beginning of the vector.
         */
        [[nodiscard]] const T* begin() const
        {
            return (const T*)m_Data;
        }

        /**
         * @brief Returns an iterator to one past the last element.
         *
         * @return Iterator to the end of the vector.
         */
        [[nodiscard]] T* end()
        {
            return (T*)m_Data + m_Size;
        }

        /**
         * @brief Returns a const iterator to one past the last element.
         *
         * @return Const iterator to the end of the vector.
         */
        [[nodiscard]] const T* end() const
        {
            return (const T*)m_Data + m_Size;
        }

    private:
        /**
         * @brief Changes the capacity of the vector to the specified value.
         *
         * Allocates a new buffer and copies/moves existing elements. Old buffer is freed.
         *
         * @param newCapacity The new capacity for the vector.
         */
        void ChangeCapacity(size_t newCapacity)
        {
            if (newCapacity == 0)
                newCapacity = 1;
            if (newCapacity <= m_Capacity)
                return;
            
            void* oldData = m_Data;
            
            // Use malloc for better portability, alignment should be handled by the allocator for most types
            if constexpr (alignof(T) <= sizeof(void*))
            {
                m_Data = malloc(sizeof(T) * newCapacity);
            }
            else
            {
                // For types with special alignment requirements, use aligned_alloc if available
                m_Data = std::aligned_alloc(alignof(T), sizeof(T) * newCapacity);
            }

            if (m_Size > 0 && oldData != nullptr)
            {
                for (size_t i = 0; i < m_Size; i++)
                {
                    if constexpr (std::is_move_constructible_v<T>)
                    {
                        new((T*)m_Data + i) T(VulkanHelper::Move(*(((T*)oldData) + i))); // Move existing elements to new buffer
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<T>, "T must be copy or move constructible");
                        new((T*)m_Data + i) T(*(((T*)oldData) + i)); // Copy existing elements to new buffer
                    }
                }

                // Destroy old elements
                for (size_t i = 0; i < m_Size; i++)
                {
                    ((T*)oldData)[i].~T();
                }
                free(oldData);
            }

            m_Capacity = newCapacity;
        }

        template<typename ... Args>
        void ConstructAt(size_t index, Args&&... args)
        {
            new ((T*)m_Data + index) T(std::forward<Args>(args)...); // Placement new to construct T in the allocated memory
        }

        void* m_Data;      ///< Pointer to the allocated memory buffer.
        size_t m_Size;     ///< Number of elements currently in the vector.
        size_t m_Capacity; ///< Allocated capacity of the vector.

        static constexpr size_t GrowFactor = 2; ///< Factor by which the capacity is increased when resizing.
    };
}