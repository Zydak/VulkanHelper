#include "pch.h"
#include "DeleteQueue.h"

#include "Vulkan/Device.h"

namespace VulkanHelper
{

	void DeleteQueue::Init(const CreateInfo& info)
	{
		s_Device = info.Device;
		s_FramesInFlight = info.FramesInFlight;
	}

	void DeleteQueue::Destroy()
	{
		for (int i = 0; i < (int)s_FramesInFlight + 1; i++)
		{
			UpdateQueue();
		}

		s_FramesInFlight = 0;
	}

	void DeleteQueue::ClearQueue()
	{
		for (int i = 0; i <= (int)s_FramesInFlight; i++)
		{
			UpdateQueue();
		}
	}

	void DeleteQueue::UpdateQueue()
	{
		s_Mutex.lock();
		// Pipelines
		for (int i = 0; i < s_PipelineQueue.size(); i++)
		{
			if (s_PipelineQueue[i].second == 0)
			{
				vkDestroyPipeline(s_Device->GetHandle(), s_PipelineQueue[i].first.Handle, nullptr);
				vkDestroyPipelineLayout(s_Device->GetHandle(), s_PipelineQueue[i].first.Layout, nullptr);

				s_PipelineQueue.erase(s_PipelineQueue.begin() + i);
				i = -1; // Go back to the beginning of the vector
			}
			else
			{
				s_PipelineQueue[i].second--;
			}
		}

		// Descriptor Sets
		for (int i = 0; i < s_SetQueue.size(); i++)
		{
			if (s_SetQueue[i].second == 0)
			{
				vkDestroyDescriptorSetLayout(s_Device->GetHandle(), s_SetQueue[i].first, nullptr);

				s_SetQueue.erase(s_SetQueue.begin() + i);
				i = -1; // Go back to the beginning of the vector
			}
			else
			{
				s_SetQueue[i].second--;
			}
		}

		// Images
		for (int i = 0; i < s_ImageQueue.size(); i++)
		{
			if (s_ImageQueue[i].second == 0)
			{
				vkDestroyImageView(s_Device->GetHandle(), s_ImageQueue[i].first.View, nullptr);

				vmaDestroyImage(s_Device->GetAllocator(), s_ImageQueue[i].first.Handle, *s_ImageQueue[i].first.Allocation);

				delete s_ImageQueue[i].first.Allocation;

				s_ImageQueue.erase(s_ImageQueue.begin() + i);
				i = -1; // Go back to the beginning of the vector
			}
			else
			{
				s_ImageQueue[i].second--;
			}
		}

		// Buffers
		for (int i = 0; i < s_BufferQueue.size(); i++)
		{
			auto& buf = s_BufferQueue[i];
			if (buf.second == 0)
			{
				// Destroy the Vulkan buffer and deallocate the buffer memory.
				vmaDestroyBuffer(s_Device->GetAllocator(), buf.first.Handle, *buf.first.Allocation);

				delete buf.first.Allocation;

				s_BufferQueue.erase(s_BufferQueue.begin() + i);
				i = -1; // Go back to the beginning of the vector
			}
			else
			{
				buf.second--;
			}
		}

		s_Mutex.unlock();
	}

	void DeleteQueue::DeletePipeline(const Pipeline& pipeline)
	{
		PipelineInfo info{};
		info.Handle = pipeline.GetPipeline();
		info.Layout = pipeline.GetPipelineLayout();

		s_Mutex.lock();
		s_PipelineQueue.emplace_back(std::make_pair(info, s_FramesInFlight));
		s_Mutex.unlock();
	}

	void DeleteQueue::DeleteImage(Image& image)
	{
		ImageInfo info{};
		info.Handle = image.GetImage();
		info.View = image.GetImageView();
		info.Allocation = image.GetAllocation();

		s_Mutex.lock();
		s_ImageQueue.emplace_back(std::make_pair(info, s_FramesInFlight));
		s_Mutex.unlock();
	}

	void DeleteQueue::DeleteBuffer(Buffer& buffer)
	{
		BufferInfo info{};
		info.Handle = buffer.GetHandle();
		info.Allocation = buffer.GetAllocation();

		s_Mutex.lock();
		s_BufferQueue.emplace_back(std::make_pair(info, s_FramesInFlight));
		s_Mutex.unlock();
	}

	void DeleteQueue::DeleteDescriptorSetLayout(DescriptorSetLayout& set)
	{
		s_Mutex.lock();
		s_SetQueue.emplace_back(std::make_pair(set.GetDescriptorSetLayoutHandle(), s_FramesInFlight));
		s_Mutex.unlock();
	}

}