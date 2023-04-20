#pragma once

#include "lve_camera.hpp"
#include "lve_game_object.hpp"

//lib
#include <vulkan/vulkan.h>

namespace lve {

#define MAX_LIGHTS 10

	struct PointLight {
		//Using vec4 to keep memory alignment as simple as posible
		glm::vec4 position{}; //ignore w
		glm::vec4 colour{}; // w -> intensity

	};

	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };

		glm::vec4 ambientLightColour{ 1.f, 1.f, 1.f, 0.02f };
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		LveCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		LveGameObject::Map& gameObjects;
	};

}//namespace lve