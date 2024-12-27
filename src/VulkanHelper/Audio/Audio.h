#pragma once

#include "portaudio.h"
#include "sndfile.h"

#include <string>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <limits>
#include <cstdint>

namespace VulkanHelper
{
	class Audio
	{
	public:
		static void Init();
		static void Destroy();

		struct AudioInfo;

		class AudioHandle
		{
		public:
			AudioHandle() = default;
			~AudioHandle() = default;
			uint64_t ID = std::numeric_limits<uint64_t>::max();

			std::shared_ptr<AudioInfo> GetAudioInfo() const { return Audio::GetAudioInfo(*this); }

			size_t Hash() const { return std::hash<uint64_t>{}(ID); }

			AudioHandle(const AudioHandle& other) = default;
			AudioHandle(AudioHandle&& other) noexcept = default;
			AudioHandle& operator=(const AudioHandle& other) = default;
			AudioHandle& operator=(AudioHandle&& other) noexcept = default;

			bool operator==(const AudioHandle& other) const { return ID == other.ID; }
			bool operator==(uint64_t other) const { return ID == other; }
		};

		struct AudioHandleHash
		{
			std::size_t operator()(const AudioHandle& handle) const noexcept
			{
				return handle.Hash();
			}
		};

		struct AudioInfo
		{
		public:
			std::atomic<float> Volume = 1.0f;
			std::atomic<bool> Paused = false;
			std::atomic<bool> Terminate = false;
			std::atomic<bool> Loop = false;

		private:
			SNDFILE* File = nullptr;
			uint32_t Channels = 1;
			AudioHandle Handle;

			friend class Audio;
		};

		static AudioHandle PlayAudio(const std::string& filePath, float volume = 1.0f, bool loop = false);

		static void inline SetMasterVolume(float volume) { m_MasterVolume = volume; }
		static float inline GetMasterVolume() { return m_MasterVolume; }

		static std::shared_ptr<AudioInfo> GetAudioInfo(AudioHandle handle);
		static bool IsHandleValid(AudioHandle handle);

	private:
		static int AudioCallback(const void* inputBuffer, void* outputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void* userData);

		static void DestroyAudioInfo(AudioHandle handle);

		static inline std::mutex m_Mutex;
		static inline std::unordered_map<AudioHandle, std::shared_ptr<AudioInfo>, AudioHandleHash> m_AudioFiles;

		Audio() = delete;
		~Audio() = delete;

		static inline std::atomic<float> m_MasterVolume = 1.0f;
		static inline bool m_Initialized = false;
	};
}