#!/bin/bash

/home/user/VulkanSDK/x86_64/bin/glslc shader.vert -o vert.spv
/home/user/VulkanSDK/x86_64/bin/glslc shader.frag -o frag.spv
/home/user/VulkanSDK/x86_64/bin/glslc sky_shader.vert -o sky_vert.spv
/home/user/VulkanSDK/x86_64/bin/glslc sky_shader.frag -o sky_frag.spv