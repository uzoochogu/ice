#include "ice_worker_threads.hpp"

#include "multithreading/ice_jobs.hpp"
namespace ice_threading {

WorkerThread::WorkerThread(WorkQueue &work_queue, bool &done,
                           vk::CommandBuffer command_buffer, vk::Queue queue)
    : work_queue(work_queue),
      done(done),
      command_buffer(command_buffer),
      queue(queue) {}

void WorkerThread::operator()() {
  work_queue.lock.lock();
#ifndef NDEBUG
  std::cout << "----    Thread is ready to go.    ----" << std::endl;
#endif
  work_queue.lock.unlock();

  while (!done) {
    if (!work_queue.lock.try_lock()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    if (work_queue.done()) {
      work_queue.lock.unlock();
      continue;
    }

    Job *pending_job = work_queue.get_next();

    if (pending_job == nullptr) {
      work_queue.lock.unlock();
      continue;
    }
#ifndef NDEBUG
    std::cout << "----    Working on a job.    ----" << std::endl;
#endif
    pending_job->status = JobStatus::IN_PROGRESS;
    work_queue.lock.unlock();
    pending_job->execute(command_buffer, queue);
  }
#ifndef NDEBUG
  std::cout << "----    Thread done.    ----" << std::endl;
#endif
}
}  // namespace ice_threading
