#pragma once

namespace VulkanHelper
{
    /**
     * @brief A RAII wrapper for a unique pointer.
     *
     * Provides a simple interface to manage the lifetime of dynamically allocated objects.
     *
     * @note It's made instead of using std::unique_ptr to avoid ABI issues with different C++ standard library implementations.
     * Since I want this to work on multiple platforms, I don't want to rely on the standard library implementation.
     * And apparently, that's a really good practice when making a library anyway, it's mentioned almost everywhere, so who am I to argue?
     *
     * @tparam T The type of the pointer stored in the UniquePtr.
     */
    template<typename T>
    class UniquePtr
    {
    public:
        /**
         * @brief Constructs a UniquePtr that takes ownership of the given pointer.
         *
         * @param ptr Pointer to the object to manage.
         */
        explicit UniquePtr(T* ptr = nullptr)
            : m_Ptr(ptr)
        {}

        /**
         * @brief Destructor. Deletes the managed object.
         */
        ~UniquePtr()
        {
            delete m_Ptr;
        }

        /**
         * @brief Move constructor. Transfers ownership from another UniquePtr.
         *
         * @param other The UniquePtr to move from.
         */
        UniquePtr(UniquePtr&& other) noexcept
            : m_Ptr(other.m_Ptr)
        {
            other.m_Ptr = nullptr;
        }

        /**
         * @brief Move assignment operator. Transfers ownership from another UniquePtr.
         *
         * @param other The UniquePtr to move from.
         * @return Reference to this UniquePtr.
         */
        UniquePtr& operator=(UniquePtr&& other) noexcept
        {
            if (this != &other)
            {
                delete m_Ptr; // Delete current object
                m_Ptr = other.m_Ptr; // Transfer ownership
                other.m_Ptr = nullptr; // Nullify the moved-from pointer
            }
            return *this;
        }

        /**
         * @brief Deleted copy constructor to prevent copying.
         */
        UniquePtr(const UniquePtr&) = delete;
        /**
         * @brief Deleted copy assignment operator to prevent copying.
         */
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr& operator=(T* ptr)
        {
            if (m_Ptr != ptr)
            {
                delete m_Ptr; // Delete current object
                m_Ptr = ptr; // Take ownership of new pointer
            }
            return *this;
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

        /*
         * @brief Returns a pointer to the managed object, allowing access to its members.
         *
         * @return Pointer to the managed object, or nullptr if empty.
        */
        T* operator->() const
        {
            return m_Ptr;
        }

        /**
         * @brief Dereferences the UniquePtr to access the managed object.
         *
         * @return Reference to the managed object.
         */
        [[nodiscard]] T& operator*() const
        {
            return *m_Ptr;
        }
    private:
        T* m_Ptr;
    };
}