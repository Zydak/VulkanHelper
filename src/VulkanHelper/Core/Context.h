#pragma once

namespace VulkanHelper
{
	class Window;

	// Struct passed into most objects. It contains pointers to objects that are more or less "global".
	// I wrap this object in a struct just so that functions won't have a million parameters for these objects,
	// there's just one struct. For now It's just window, but this list might grow.
	struct VulkanHelperContext
	{
		Window* Window;
	};
}