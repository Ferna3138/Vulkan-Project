#include "first_app.hpp"
#include "lve_camera.hpp"
#include "keyboard_modement_controller.hpp"
#include "lve_buffer.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <chrono>
#include <array>
#include <cassert>
#include <numeric>

#include "lve_texture.hpp"

#define MAX_FRAME_TIME 120

namespace lve {

	FirstApp::FirstApp() {
        globalPool = LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
        loadGameObjects(); 
	}

	FirstApp::~FirstApp() {}


    void FirstApp::run() {/*

        LveBuffer globalUboBuffer{
            lveDevice,
            sizeof(GlobalUbo),
            LveSwapChain::MAX_FRAMES_IN_FLIGHT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            lveDevice.properties.limits.minUniformBufferOffsetAlignment
        };
        globalUboBuffer.map();*/

        std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<LveBuffer>(
                lveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        
        Texture woodTexture = Texture(lveDevice, "C:\\Users\\fernandoedelstein\\Documents\\Vulkan\\Project1\\Textures\\image.png");

        VkDescriptorImageInfo woodImageInfo{};
        woodImageInfo.sampler = woodTexture.getSampler();
        woodImageInfo.imageView = woodTexture.getImageView();
        woodImageInfo.imageLayout = woodTexture.getImageLayout();

        Texture metalTexture = Texture(lveDevice, "C:\\Users\\fernandoedelstein\\Documents\\Vulkan\\Project1\\Textures\\metal.jpg");

        VkDescriptorImageInfo metalImageInfo{};
        metalImageInfo.sampler = metalTexture.getSampler();
        metalImageInfo.imageView = metalTexture.getImageView();
        metalImageInfo.imageLayout = metalTexture.getImageLayout();



        std::vector<VkDescriptorSet>globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            LveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &woodImageInfo)
                .build(globalDescriptorSets[i]);
        }

		SimpleRenderSystem simpleRenderSystem{ 
            lveDevice,
            lveRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()};

        PointLightSystem pointLightSystem{
            lveDevice,
            lveRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout() };

        LveCamera camera{};
           
        //camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = LveGameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f;

        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!lveWindow.shouldClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = 
                std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            //frameTime = glm::min(frameTime, MAX_FRAME_TIME);

            cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            float aspect = lveRenderer.getAspectRatio();

            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
			
            if (auto commandBuffer = lveRenderer.beginFrame()) {
                int frameIndex = lveRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                //Update objects and memory
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                //Render
                lveRenderer.beginSwapChainRenderPass(commandBuffer);
				
                //Order matters
                simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
				
                lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(lveDevice.device());
	}



	void FirstApp::loadGameObjects() {
        std::shared_ptr<LveModel> lveModel =
            LveModel::createModelFromFile(lveDevice, "C:\\Users\\fernandoedelstein\\Documents\\Vulkan\\Project1\\models\\flat_vase.obj");

        auto flatVase = LveGameObject::createGameObject();
        flatVase.model = lveModel;
        flatVase.transform.translation = {-.5f, .5f, 0.f };
        flatVase.transform.scale = {3.f, 1.5f, 3.f};
        gameObjects.emplace(flatVase.getId(),std::move(flatVase));


        lveModel =
            LveModel::createModelFromFile(lveDevice, "C:\\Users\\fernandoedelstein\\Documents\\Vulkan\\Project1\\models\\smooth_vase.obj");

        auto smoothVase = LveGameObject::createGameObject();
        smoothVase.model = lveModel;
        smoothVase.transform.translation = { .5f, .5f, 0.f };
        smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
        gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

        lveModel =
            LveModel::createModelFromFile(lveDevice, "C:\\Users\\fernandoedelstein\\Documents\\Vulkan\\Project1\\models\\quad.obj");

        auto floor = LveGameObject::createGameObject();
        floor.model = lveModel;
        floor.transform.translation = { 0.f, .5f, 0.f };
        floor.transform.scale = { 3.f, 1.f, 3.f };
        gameObjects.emplace(floor.getId(), std::move(floor));
        

        std::vector<glm::vec3> lightColors{
            {1.f, .1f, .1f},
            {.1f, .1f, 1.f},
            {.1f, 1.f, .1f},
            {1.f, 1.f, .1f},
            {.1f, 1.f, 1.f},
            {1.f, 1.f, 1.f}  //
        };

        for (int i = 0;i < lightColors.size();i++) {
            auto pointLight = LveGameObject::makePointLight(0.5f);
            pointLight.colour = lightColors[i];
            auto rotateLight = glm::rotate(
                glm::mat4(1.f),
                (i * glm::two_pi<float>()) / lightColors.size(),
                { 0.f,-1.f,0.f }); //Up vector on which to rotate

            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, -1.f));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }


	}

}//namespace lve