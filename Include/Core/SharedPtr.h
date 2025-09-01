#pragma once

#include "Core/Move.h"

#include <cstddef>
#include <atomic>

#define USE_STD_SHARED_PTR 1

#if USE_STD_SHARED_PTR
#include <memory>
#endif

namespace VulkanHelper
{
#if USE_STD_SHARED_PTR

    // I have absolutely no fucking clue how is my implementation unsafe, but when using it in release mode it causes some random
    // undebuggable crashes in malloc only when compiling with GCC. So I'm just going to use std::shared_ptr for now
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    #define MakeShared std::make_shared

#else
    /**
     * @brief A RAII wrapper for a shared pointer.
     *
     * Provides reference counting to manage the lifetime of dynamically allocated objects
     * across multiple owners. When the reference count reaches zero, the object is deleted.
     *
     * @note It's made instead of using std::shared_ptr to avoid ABI issues with different C++ standard library implementations.
     * Since I want this to work on multiple platforms, I don't want to rely on the standard library implementation.
     * And apparently, that's a really good practice when making a library anyway, it's mentioned almost everywhere, so who am I to argue?
     *
     * @tparam T The type of the pointer stored in the SharedPtr.
     */
    template<typename T>
    class SharedPtr
    {
    private:
        T* m_Ptr;
        std::atomic<size_t>* m_RefCount;

        void Release()
        {
            if (m_RefCount && m_RefCount->fetch_sub(1) == 1)
            {
                delete m_Ptr;
                delete m_RefCount;
            }
            m_Ptr = nullptr;
            m_RefCount = nullptr;
        }

    public:
        /**
         * @brief Constructs a SharedPtr that takes ownership of the given pointer.
         *
         * @param ptr Pointer to the object to manage. Can be nullptr.
         */
        explicit SharedPtr(T* ptr = nullptr)
            : m_Ptr(ptr), m_RefCount(ptr ? new std::atomic<size_t>(1) : nullptr)
        {}

        /**
         * @brief Copy constructor. Increases reference count.
         *
         * @param other The SharedPtr to copy from.
         */
        SharedPtr(const SharedPtr& other)
            : m_Ptr(other.m_Ptr), m_RefCount(other.m_RefCount)
        {
            if (m_RefCount)
            {
                m_RefCount->fetch_add(1);
            }
        }

        /**
         * @brief Move constructor. Transfers ownership from another SharedPtr.
         *
         * @param other The SharedPtr to move from.
         */
        SharedPtr(SharedPtr&& other) noexcept
            : m_Ptr(other.m_Ptr), m_RefCount(other.m_RefCount)
        {
            other.m_Ptr = nullptr;
            other.m_RefCount = nullptr;
        }

        /**
         * @brief Copy assignment operator. Updates reference counts appropriately.
         *
         * @param other The SharedPtr to copy from.
         * @return Reference to this SharedPtr.
         */
        SharedPtr& operator=(const SharedPtr& other)
        {
            if (this != &other)
            {
                Release(); // Release current resource
                
                m_Ptr = other.m_Ptr;
                m_RefCount = other.m_RefCount;
                
                if (m_RefCount)
                {
                    m_RefCount->fetch_add(1);
                }
            }
            return *this;
        }

        /**
         * @brief Move assignment operator. Transfers ownership from another SharedPtr.
         *
         * @param other The SharedPtr to move from.
         * @return Reference to this SharedPtr.
         */
        SharedPtr& operator=(SharedPtr&& other) noexcept
        {
            if (this != &other)
            {
                Release(); // Release current resource
                
                m_Ptr = other.m_Ptr;
                m_RefCount = other.m_RefCount;
                
                other.m_Ptr = nullptr;
                other.m_RefCount = nullptr;
            }
            return *this;
        }

        /**
         * @brief Assignment operator for raw pointer.
         *
         * @param ptr Raw pointer to take ownership of.
         * @return Reference to this SharedPtr.
         */
        SharedPtr& operator=(T* ptr)
        {
            Release(); // Release current resource
            
            m_Ptr = ptr;
            m_RefCount = ptr ? new std::atomic<size_t>(1) : nullptr;
            
            return *this;
        }

        /**
         * @brief Assignment operator for nullptr.
         *
         * @param ptr nullptr to reset the SharedPtr.
         * @return Reference to this SharedPtr.
         */
        SharedPtr& operator=(std::nullptr_t)
        {
            Release(); // Release current resource

            m_Ptr = nullptr;
            m_RefCount = nullptr;

            return *this;
        }

        /**
         * @brief Constructs a SharedPtr from a nullptr.
         *
         * This allows creating an empty SharedPtr without managing any object.
         */
        SharedPtr(std::nullptr_t)
            : m_Ptr(nullptr), m_RefCount(nullptr)
        {}

        /**
         * @brief Destructor. Decreases reference count and deletes the managed object if needed.
         */
        ~SharedPtr()
        {
            Release();
        }

        /**
         * @brief Returns a pointer to the managed object.
         *
         * @return Pointer to the managed object, or nullptr if empty.
         */
        [[nodiscard]] T* Get() const
        {
            return m_Ptr;
        }

        /**
         * @brief Returns a pointer to the managed object, allowing access to its members.
         *
         * @return Pointer to the managed object, or nullptr if empty.
         */
        T* operator->() const
        {
            return m_Ptr;
        }

        /**
         * @brief Dereferences the SharedPtr to access the managed object.
         *
         * @return Reference to the managed object.
         */
        [[nodiscard]] T& operator*() const
        {
            return *m_Ptr;
        }

        /**
         * @brief Checks if the SharedPtr is not empty.
         *
         * @return True if the SharedPtr manages an object, false otherwise.
         */
        [[nodiscard]] operator bool() const
        {
            return m_Ptr != nullptr;
        }

        /**
         * @brief Returns the current reference count.
         *
         * @return Number of SharedPtr instances sharing ownership of the managed object, or 0 if empty.
         */
        [[nodiscard]] size_t UseCount() const
        {
            return m_RefCount ? m_RefCount->load() : 0;
        }

        /**
         * @brief Checks if this is the only SharedPtr managing the object.
         *
         * @return True if this is the only owner, false otherwise.
         */
        [[nodiscard]] bool Unique() const
        {
            return UseCount() == 1;
        }

        /**
         * @brief Releases ownership of the managed object.
         *
         * After calling Reset(), the SharedPtr becomes empty and Get() returns nullptr.
         */
        void Reset()
        {
            Release();
        }

        /**
         * @brief Replaces the managed object with a new one.
         *
         * @param ptr New pointer to manage.
         */
        void Reset(T* ptr)
        {
            *this = ptr;
        }

        /**
         * @brief Swaps the contents of this SharedPtr with another.
         *
         * @param other The SharedPtr to swap with.
         */
        void Swap(SharedPtr& other) noexcept
        {
            T* tempPtr = m_Ptr;
            size_t* tempRefCount = m_RefCount;
            
            m_Ptr = other.m_Ptr;
            m_RefCount = other.m_RefCount;
            
            other.m_Ptr = tempPtr;
            other.m_RefCount = tempRefCount;
        }
    };

    /**
     * @brief Creates a SharedPtr with an object constructed in-place.
     *
     * @tparam T The type of object to create.
     * @tparam Args The types of arguments to pass to the constructor.
     * @param args Arguments to pass to the object's constructor.
     * @return SharedPtr managing the newly constructed object.
     */
    template<typename T, typename... Args>
    [[nodiscard]] SharedPtr<T> MakeShared(Args&&... args)
    {
        return SharedPtr<T>(new T(VulkanHelper::Move(args)...));
    }

    /**
     * @brief Equality comparison between two SharedPtr instances.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator==(const SharedPtr<T1>& lhs, const SharedPtr<T2>& rhs)
    {
        return lhs.Get() == rhs.Get();
    }

    /**
     * @brief Inequality comparison between two SharedPtr instances.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator!=(const SharedPtr<T1>& lhs, const SharedPtr<T2>& rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * @brief Equality comparison between SharedPtr and raw pointer.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator==(const SharedPtr<T1>& lhs, T2* rhs)
    {
        return lhs.Get() == rhs;
    }

    /**
     * @brief Equality comparison between raw pointer and SharedPtr.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator==(T1* lhs, const SharedPtr<T2>& rhs)
    {
        return lhs == rhs.Get();
    }

    /**
     * @brief Inequality comparison between SharedPtr and raw pointer.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator!=(const SharedPtr<T1>& lhs, T2* rhs)
    {
        return lhs.Get() != rhs;
    }

    /**
     * @brief Inequality comparison between raw pointer and SharedPtr.
     */
    template<typename T1, typename T2>
    [[nodiscard]] bool operator!=(T1* lhs, const SharedPtr<T2>& rhs)
    {
        return lhs != rhs.Get();
    }

    /**
     * @brief Equality comparison between SharedPtr and nullptr.
     */
    template<typename T>
    [[nodiscard]] bool operator==(const SharedPtr<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() == nullptr;
    }

    /**
     * @brief Equality comparison between nullptr and SharedPtr.
     */
    template<typename T>
    [[nodiscard]] bool operator==(std::nullptr_t, const SharedPtr<T>& rhs)
    {
        return rhs.Get() == nullptr;
    }

    /**
     * @brief Inequality comparison between SharedPtr and nullptr.
     */
    template<typename T>
    [[nodiscard]] bool operator!=(const SharedPtr<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() != nullptr;
    }

    /**
     * @brief Inequality comparison between nullptr and SharedPtr.
     */
    template<typename T>
    [[nodiscard]] bool operator!=(std::nullptr_t, const SharedPtr<T>& rhs)
    {
        return rhs.Get() != nullptr;
    }

    #endif
}
