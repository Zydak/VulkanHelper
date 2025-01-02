#pragma once
#include "pch.h"

namespace VulkanHelper
{
	namespace Bytes
	{
		static std::vector<char> ToBytes(void* data, uint32_t size)
		{
			std::vector<char> bytes(size);
			memcpy(bytes.data(), data, size);

			return bytes;
		}

		template<typename T>
		static T FromBytes(const std::vector<char>& bytes)
		{
			T object;
			memcpy(&object, bytes.data(), sizeof(T));

			return object;
		}

		static std::string NumberToHex(uint32_t number, uint8_t numberOfDigits)
		{
			std::stringstream ss;
			ss << std::hex << std::uppercase << std::setw(numberOfDigits) << std::setfill('0') << number;
			return ss.str();
		}
	}
}