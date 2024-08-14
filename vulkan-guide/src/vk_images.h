#pragma once

#include <vk_initializers.h>

namespace vkutil {
void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout currentLayout, VkImageLayout newLayout);
}; // namespace vkutil
