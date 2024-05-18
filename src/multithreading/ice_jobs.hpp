#ifndef ICE_JOBS
#define ICE_JOBS

#include "../config.hpp"
#include "../images/ice_image.hpp"
#include "../images/ice_texture.hpp"
#include "../mesh.hpp"
#include "images/ice_image.hpp"
#include <stdio.h>

namespace ice_threading {
enum class JobStatus { PENDING, IN_PROGRESS, COMPLETE };

class Job {
public:
  virtual ~Job() = default;
  JobStatus status = JobStatus::PENDING;
  Job *next = nullptr;
  virtual void execute(vk::CommandBuffer command_buffer, vk::Queue queue) = 0;
};

class MakeModel : public Job {
public:
  const char *obj_filepath;
  const char *mtl_filepath;
  glm::mat4 pre_transform;
  ice::ObjMesh &mesh;
  MakeModel(ice::ObjMesh &mesh, const char *obj_filepath,
            const char *mtl_filepath, glm::mat4 pre_transform);
  virtual void execute(vk::CommandBuffer command_buffer, vk::Queue queue) final;
};

class MakeTexture : public Job {
public:
  ice_image::TextureCreationInput texture_info;
  ice_image::Texture *texture;
  MakeTexture(ice_image::Texture *texture,
              ice_image::TextureCreationInput texture_info);
  virtual void execute(vk::CommandBuffer command_buffer, vk::Queue queue) final;
};

class WorkQueue {
public:
  Job *first = nullptr, *last = nullptr;
  size_t length = 0;
  std::mutex lock;
  void add(Job *job);
  Job *get_next();
  bool done();
  void clear();
};
} // namespace ice_threading

#endif