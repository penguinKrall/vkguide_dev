﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <camera.h>
#include <vk_descriptors.h>
#include <vk_loader.h>
#include <vk_types.h>

struct MeshNode : public Node {

  std::shared_ptr<MeshAsset> mesh;

  virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;
};

struct RenderObject {
  uint32_t indexCount;
  uint32_t firstIndex;
  VkBuffer indexBuffer;

  MaterialInstance *material;

  glm::mat4 transform;
  VkDeviceAddress vertexBufferAddress;
};

struct DrawContext {
  std::vector<RenderObject> OpaqueSurfaces;
};

struct GLTFMetallic_Roughness {
  MaterialPipeline opaquePipeline;
  MaterialPipeline transparentPipeline;

  VkDescriptorSetLayout materialLayout;

  struct MaterialConstants {
    glm::vec4 colorFactors;
    glm::vec4 metal_rough_factors;
    // padding, we need it anyway for uniform buffers
    glm::vec4 extra[14];
  };

  struct MaterialResources {
    AllocatedImage colorImage;
    VkSampler colorSampler;
    AllocatedImage metalRoughImage;
    VkSampler metalRoughSampler;
    VkBuffer dataBuffer;
    uint32_t dataBufferOffset;
  };

  DescriptorWriter writer;

  void build_pipelines(VulkanEngine *engine);
  void clear_resources(VkDevice device);

  MaterialInstance
  write_material(VkDevice device, MaterialPass pass,
                 const MaterialResources &resources,
                 DescriptorAllocatorGrowable &descriptorAllocator);
};

struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
  glm::vec4 ambientColor;
  glm::vec4 sunlightDirection; // w for sun power
  glm::vec4 sunlightColor;
};

struct ComputePushConstants {
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};

struct ComputeEffect {
  const char *name;

  VkPipeline pipeline;
  VkPipelineLayout layout;

  ComputePushConstants data;
};

struct DeletionQueue {
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()> &&function) {
    deletors.push_back(function);
  }

  void flush() {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
      (*it)(); // call functors
      // fmt::print("deletor");
    }

    deletors.clear();
  }
};

struct FrameData {
  VkSemaphore _swapchainSemaphore, _renderSemaphore;
  VkFence _renderFence;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
  DeletionQueue _deletionQueue;
  DescriptorAllocatorGrowable _frameDescriptors;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
  Camera mainCamera;

  DrawContext mainDrawContext;
  std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

  void update_scene();

  MaterialInstance defaultData;
  GLTFMetallic_Roughness metalRoughMaterial;

  GPUSceneData sceneData;

  VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

  std::vector<std::shared_ptr<MeshAsset>> testMeshes;

  VkPipelineLayout _meshPipelineLayout;
  VkPipeline _meshPipeline;

  void init_mesh_pipeline();

  std::vector<ComputeEffect> backgroundEffects;
  int currentBackgroundEffect{0};

  // immediate submit structures
  VkFence _immFence;
  VkCommandBuffer _immCommandBuffer;
  VkCommandPool _immCommandPool;

  void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);

  VkPipeline _gradientPipeline;
  VkPipelineLayout _gradientPipelineLayout;

  // DescriptorAllocator globalDescriptorAllocator;
  DescriptorAllocatorGrowable globalDescriptorAllocator;

  VkDescriptorSet _drawImageDescriptors;
  VkDescriptorSetLayout _drawImageDescriptorLayout;

  // draw resources
  AllocatedImage _drawImage;
  AllocatedImage _depthImage;

  AllocatedImage _whiteImage;
  AllocatedImage _blackImage;
  AllocatedImage _greyImage;
  AllocatedImage _errorCheckerboardImage;

  VkDescriptorSetLayout _singleImageDescriptorLayout;

  VkSampler _defaultSamplerLinear;
  VkSampler _defaultSamplerNearest;

  VmaAllocator _allocator;

  DeletionQueue _mainDeletionQueue;

  FrameData _frames[FRAME_OVERLAP];

  FrameData &get_current_frame() {
    return _frames[_frameNumber % FRAME_OVERLAP];
  };

  VkQueue _graphicsQueue;
  uint32_t _graphicsQueueFamily;

  bool _isInitialized{false};
  int _frameNumber{0};
  bool stop_rendering{false};
  bool bUseValidationLayers{true};
  // VkExtent2D _windowExtent{800, 600};
  VkExtent2D _drawExtent{800, 600};
  float renderScale = 1.f;

  VkInstance _instance;                      // Vulkan library handle
  VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
  VkPhysicalDevice _chosenGPU;               // GPU chosen as the default device
  VkDevice _device;                          // Vulkan device for commands
  VkSurfaceKHR _surface;                     // Vulkan window surface

  VkSwapchainKHR _swapchain;
  VkFormat _swapchainImageFormat;

  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;
  VkExtent2D _swapchainExtent;

  struct SDL_Window *_window{nullptr};

  static VulkanEngine &Get();

  bool resize_requested{false};

  // initializes everything in the engine
  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  // draw background
  void draw_background(VkCommandBuffer cmd);

  // draw imgui
  void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

  // draw geometry
  void draw_geometry(VkCommandBuffer cmd);

  // run main loop
  void run();

  GPUMeshBuffers uploadMesh(std::span<uint32_t> indices,
                            std::span<Vertex> vertices);

  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
  AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage,
                                VmaMemoryUsage memoryUsage);
  void destroy_buffer(const AllocatedBuffer &buffer);
  void resize_swapchain();
  AllocatedImage create_image(VkExtent3D size, VkFormat format,
                              VkImageUsageFlags usage, bool mipmapped = false);
  AllocatedImage create_image(void *data, VkExtent3D size, VkFormat format,
                              VkImageUsageFlags usage, bool mipmapped = false);
  void destroy_image(const AllocatedImage &img);

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void init_descriptors();
  void init_pipelines();
  void init_background_pipelines();
  void init_imgui();
  void init_default_data();
};
