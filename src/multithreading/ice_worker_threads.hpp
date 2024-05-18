#ifndef ICE_WORKER_THREADS_HPP
#define ICE_WORKER_THREADS_HPP

#include "ice_jobs.hpp"
namespace ice_threading {
class WorkerThread {
public:
  bool &done;
  WorkQueue &work_queue;
  vk::CommandBuffer command_buffer;
  vk::Queue queue;

  WorkerThread(WorkQueue &work_queue, bool &done,
               vk::CommandBuffer command_buffer, vk::Queue queue);

  void operator()();
};
} // namespace ice_threading

#endif