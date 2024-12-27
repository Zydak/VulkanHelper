#pragma once
#include "pch.h"

#include "Audio.h"

#include "Utility/Utility.h"

#include "glm/glm.hpp"

#include "Asset/Asset.h"

namespace VulkanHelper
{
	int Audio::AudioCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData)
	{
		float* out = (float*)outputBuffer;
		Audio::AudioInfo* sndfile = (Audio::AudioInfo*)userData;

		if (sndfile->Terminate)
		{
			sf_close(sndfile->File);
			Audio::DestroyAudioInfo(sndfile->Handle);
			return PaStreamCallbackResult::paComplete;
		}
		else if (sndfile->Paused)
		{
			memset(out, 0, framesPerBuffer * sndfile->Channels * sizeof(float));

			return PaStreamCallbackResult::paContinue;
		}

		uint32_t numFramesRead = (uint32_t)sf_readf_float(sndfile->File, out, framesPerBuffer);
		if (numFramesRead < (uint32_t)framesPerBuffer)
		{
			if (sndfile->Loop)
			{
				sf_seek(sndfile->File, 0, SEEK_SET);
				numFramesRead = (uint32_t)sf_readf_float(sndfile->File, out, framesPerBuffer);
			}
			else
			{
				// Apply volume to the remaining samples
				for (uint32_t i = 0; i < numFramesRead * sndfile->Channels; i++)
				{
					out[i] *= sndfile->Volume * Audio::GetMasterVolume();
				}

				sf_close(sndfile->File);
				Audio::DestroyAudioInfo(sndfile->Handle);
				return PaStreamCallbackResult::paComplete;
			}
		}

		// Apply volume to the samples
		for (uint32_t i = 0; i < numFramesRead * sndfile->Channels; i++)
		{
			out[i] *= sndfile->Volume * Audio::GetMasterVolume();
		}

		return PaStreamCallbackResult::paContinue;
	}

	std::shared_ptr<VulkanHelper::Audio::AudioInfo> Audio::GetAudioInfo(AudioHandle handle)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		auto iter = m_AudioFiles.find(handle);
		if (iter == m_AudioFiles.end())
		{
			return nullptr;
		}
		else
		{
			return iter->second;
		}
	}

	bool Audio::IsHandleValid(AudioHandle handle)
	{
		m_Mutex.lock();
		bool valid =  m_AudioFiles.find(handle) != m_AudioFiles.end();
		m_Mutex.unlock();

		return valid;
	}

	void Audio::DestroyAudioInfo(AudioHandle handle)
	{
		m_Mutex.lock();
		m_AudioFiles.erase(handle);
		m_Mutex.unlock();
	}

	void Audio::Init()
	{
		if (m_Initialized)
			Destroy();

		PaError error = Pa_Initialize();
		VK_CORE_ASSERT(error == paNoError, "PortAudio failed to initialize. {}", Pa_GetErrorText(error));

		m_Initialized = true;
	}

	void Audio::Destroy()
	{
		m_Initialized = false;

		PaError error = Pa_Terminate();
		VK_CORE_ASSERT(error == paNoError, "PortAudio failed to shutdown. {}", Pa_GetErrorText(error));

	}

	Audio::AudioHandle Audio::PlayAudio(const std::string& filePath, float volume /*= 1.0f*/, bool loop /*= false*/)
	{
		SF_INFO info;
		SNDFILE* audioFile = sf_open(filePath.c_str(), SFM_READ, &info);
		if (!audioFile)
		{
			int errnum = 0;
			sf_error_number(errnum);
			const char* errmsg = sf_strerror(audioFile);
			VK_CORE_ERROR("Error opening audio file: {0} - {1}", errnum, errmsg);
			return {};
		}

		std::shared_ptr<AudioInfo> aFile = std::make_shared<AudioInfo>(); // Can't use unique_ptr because the callback has to match the signature of PortAudio one
		aFile->File = audioFile;
		aFile->Channels = info.channels;
		aFile->Loop = loop;
		aFile->Volume = volume;
		PaStream* stream;
		PaError err;

		err = Pa_OpenDefaultStream(&stream,
			0,
			info.channels,
			paFloat32,
			info.samplerate,
			paFramesPerBufferUnspecified,
			Audio::AudioCallback,
			aFile.get());

		VK_CORE_ASSERT(err == paNoError, "PortAudio failed to open stream. {}", Pa_GetErrorText(err));
		err = Pa_StartStream(stream);
		VK_CORE_ASSERT(err == paNoError, "PortAudio failed to start stream. {}", Pa_GetErrorText(err));

		static std::atomic<uint64_t> counter = 0;
		AudioHandle handle;
		handle.ID = counter;
		counter++;

		aFile->Handle = handle;

		m_AudioFiles[handle] = aFile;

		return handle;
	}
}
