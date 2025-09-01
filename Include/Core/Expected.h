#pragma once

#include "Log/Log.h"
#include "Move.h"

namespace VulkanHelper
{
    template<typename E>
    class Unexpected
    {
    public:
        E m_Error;
        explicit Unexpected(const E& e) : m_Error(e) {}
        explicit Unexpected(E&& e) : m_Error(VulkanHelper::Move(e)) {}

        const E& GetError() const { return m_Error; }
        E& GetError() { return m_Error; }
    };

    /**
     * @brief A type that contains either a value or an error.
     *
     * This class is a minimal implementation of the C++23 std::expected type, used for error handling.
     *
     * @note It's made instead of using std::expected to avoid ABI issues with different C++ standard library implementations.
     * Since I want this to work on multiple platforms, I don't want to rely on the standard library implementation.
     * And apparently, that's a really good practice when making a library anyway, it's mentioned almost everywhere, so who am I to argue?
     *
     * @tparam T The type of the value.
     * @tparam E The type of the error.
     */
    template<typename T, typename E>
    class Expected {
    private:
        union {
            T m_Value;
            E m_Error;
        };
        bool m_HasValue;

    public:
        /**
         * @brief Constructs an Expected containing a value.
         * @param value The value to store.
         */
        Expected(const T& value) : m_Value(value), m_HasValue(true) {}

        /**
         * @brief Constructs an Expected by moving a value.
         * @param value The value to move into the Expected.
         */
        Expected(T&& value) : m_Value(VulkanHelper::Move(value)), m_HasValue(true) {}

        /**
         * @brief Constructs an Expected containing an error.
         * @param error The Unexpected error to store (copied).
         */
        Expected(const Unexpected<E>& error) : m_Error(error.GetError()), m_HasValue(false) {}

        /**
         * @brief Constructs an Expected containing an error by moving.
         * @param error The Unexpected error to store (moved).
         */
        Expected(Unexpected<E>&& error) : m_Error(VulkanHelper::Move(error.GetError())), m_HasValue(false) {}

        /**
         * @brief Copy constructor. Copies value or error from another Expected.
         * @param other The Expected to copy from.
         */
        Expected(const Expected& other) : m_HasValue(other.m_HasValue) {
            if (m_HasValue) {
                new(&m_Value) T(other.m_Value);
            } else {
                new(&m_Error) E(other.m_Error);
            }
        }

        /**
         * @brief Move constructor. Moves value or error from another Expected.
         * @param other The Expected to move from.
         */
        Expected(Expected&& other) : m_HasValue(other.m_HasValue) {
            if (m_HasValue) {
                new(&m_Value) T(VulkanHelper::Move(other.m_Value));
            } else {
                new(&m_Error) E(VulkanHelper::Move(other.m_Error));
            }
        }

        /**
         * @brief Destructor. Destroys the contained value or error.
         */
        ~Expected() {
            if (m_HasValue) {
                m_Value.~T();
            } else {
                m_Error.~E();
            }
        }

        // Assignment operators
        /**
         * @brief Copy assignment operator. Copies value or error from another Expected.
         * @param other The Expected to copy from.
         * @return Reference to this Expected.
         */
        Expected& operator=(const Expected& other) {
            if (this != &other) {
                m_HasValue = other.m_HasValue;
                if (m_HasValue) {
                    new(&m_Value) T(other.m_Value);
                } else {
                    new(&m_Error) E(other.m_Error);
                }
            }
            return *this;
        }

        /**
         * @brief Move assignment operator. Moves value or error from another Expected.
         * @param other The Expected to move from.
         * @return Reference to this Expected.
         */
        Expected& operator=(Expected&& other) {
            if (this != &other) {
                m_HasValue = other.m_HasValue;
                if (m_HasValue) {
                    new(&m_Value) T(VulkanHelper::Move(other.m_Value));
                } else {
                    new(&m_Error) E(VulkanHelper::Move(other.m_Error));
                }
            }
            return *this;
        }

        /**
         * @brief Checks if the Expected contains a value.
         * @return True if a value is present, false if it contains an error.
         */
        bool HasValue() const { return m_HasValue; }
        /**
         * @brief Checks if the Expected contains a value (conversion to bool).
         * @return True if a value is present, false if it contains an error.
         */
        explicit operator bool() const { return m_HasValue; }

        // Access value
        /**
         * @brief Accesses the contained value (lvalue).
         * @return Reference to the contained value.
         * @throws If the Expected contains an error, triggers VH_ASSERT.
         */
        T& Value() & {
            VH_ASSERT(m_HasValue, "Expected contains error. Cannot access value");
            return m_Value;
        }

        /**
         * @brief Accesses the contained value (const lvalue).
         * @return Const reference to the contained value.
         * @throws If the Expected contains an error, triggers VH_ASSERT.
         */
        const T& Value() const & {
            VH_ASSERT(m_HasValue, "Expected contains error. Cannot access value");
            return m_Value;
        }

        /**
         * @brief Accesses the contained value (rvalue).
         * @return Rvalue reference to the contained value.
         * @throws If the Expected contains an error, triggers VH_ASSERT.
         */
        T&& Value() && {
            VH_ASSERT(m_HasValue, "Expected contains error. Cannot access value");
            return VulkanHelper::Move(m_Value);
        }

        // Access error
        /**
         * @brief Accesses the contained error (lvalue).
         * @return Reference to the contained error.
         * @throws If the Expected contains a value, triggers VH_ASSERT.
         */
        E& Error() & {
            VH_ASSERT(!m_HasValue, "Expected contains value. Cannot access error");
            return m_Error;
        }

        /**
         * @brief Accesses the contained error (const lvalue).
         * @return Const reference to the contained error.
         * @throws If the Expected contains a value, triggers VH_ASSERT.
         */
        const E& Error() const & {
            VH_ASSERT(!m_HasValue, "Expected contains value. Cannot access error");
            return m_Error;
        }

        /**
         * @brief Accesses the contained error (rvalue).
         * @return Rvalue reference to the contained error.
         * @throws If the Expected contains a value, triggers VH_ASSERT.
         */
        E&& Error() && {
            VH_ASSERT(!m_HasValue, "Expected contains value. Cannot access error");
            return VulkanHelper::Move(m_Error);
        }

        // Dereference operators
        /**
         * @brief Dereference operator to access the contained value (lvalue).
         * @return Reference to the contained value.
         */
        T& operator*() & { return Value(); }

        /**
         * @brief Dereference operator to access the contained value (const lvalue).
         * @return Const reference to the contained value.
         */
        const T& operator*() const & { return Value(); }

        /**
         * @brief Dereference operator to access the contained value (rvalue).
         * @return Rvalue reference to the contained value.
         */
        T&& operator*() && { return VulkanHelper::Move(Value()); }

        /**
         * @brief Arrow operator to access the contained value's members.
         * @return Pointer to the contained value.
         */
        T* operator->() { return &Value(); }

        /**
         * @brief Arrow operator to access the contained value's members (const).
         * @return Const pointer to the contained value.
         */
        const T* operator->() const { return &Value(); }
    };
}