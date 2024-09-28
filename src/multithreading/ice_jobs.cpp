#include "./ice_jobs.hpp"

#include "images/ice_image.hpp"

namespace ice_threading {

// MakeModel
MakeModel::MakeModel(ice::ObjMesh &mesh, const char *obj_filepath,
                     const char *mtl_filepath, glm::mat4 pre_transform)
    : mesh(mesh), pre_transform(pre_transform) {
  this->obj_filepath = obj_filepath;
  this->mtl_filepath = mtl_filepath;
}

// NOLINTBEGIN (misc-unused-parameters)
void MakeModel::execute(vk::CommandBuffer command_buffer, vk::Queue queue) {
  mesh.load(obj_filepath, mtl_filepath, pre_transform);
  status = JobStatus::COMPLETE;
}
// NOLINTEND (misc-unused-parameters)

// MakeTexture
MakeTexture::MakeTexture(std::shared_ptr<ice_image::Texture> texture,
                         ice_image::TextureCreationInput texture_info)
    : texture(std::move(texture)), texture_info(std::move(texture_info)) {}

void MakeTexture::execute(vk::CommandBuffer command_buffer, vk::Queue queue) {
  texture_info.command_buffer = command_buffer;
  texture_info.queue = queue;
  texture->load(texture_info);
  status = JobStatus::COMPLETE;
}

// WorkQueue
void WorkQueue::add(Job *job) {
  if (length == 0) {
    first = job;
    last = job;
  } else {
    last->next = job;
    last = job;
  }

  length += 1;
}

Job *WorkQueue::get_next() const {
  Job *current = first;
  while (current) {
    if (current->status == JobStatus::PENDING) {
      return current;
    }

    current = current->next;
  }

  return nullptr;
}

bool WorkQueue::done() const {
  Job *current = first;
  while (current) {
    if (current->status != JobStatus::COMPLETE) {
      return false;
    }

    current = current->next;
  }

  return true;
}

void WorkQueue::clear() {
  if (length == 0) {
    return;
  }

  Job *current = first;
  while (current) {
    Job *previous = current;
    current = current->next;
    delete previous;
  }

  first = nullptr;
  last = nullptr;
  length = 0;
}
}  // namespace ice_threading
