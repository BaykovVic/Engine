#include <algorithm>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "sky/core/runtime_services.hpp"

namespace sky::core {
namespace {

class ThreadPoolScheduler final : public IJobScheduler {
public:
    explicit ThreadPoolScheduler(unsigned threadCount) {
        if (threadCount == 0) {
            threadCount = std::max(1u, std::thread::hardware_concurrency());
        }
        workers_.reserve(threadCount);
        for (unsigned i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPoolScheduler() override {
        {
            const std::scoped_lock lock(mutex_);
            stopping_ = true;
        }
        wakeWorkers_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    JobHandle schedule(Job job) override {
        return enqueue(std::move(job), JobHandle{});
    }

    JobHandle scheduleAfter(JobHandle dependency, Job job) override {
        return enqueue(std::move(job), dependency);
    }

    void wait(JobHandle job) override {
        std::unique_lock lock(mutex_);
        jobDone_.wait(lock, [&] { return !pending_.contains(job.value); });
    }

private:
    struct PendingJob {
        Job job;
        std::uint64_t dependency = 0;
    };

    JobHandle enqueue(Job job, JobHandle dependency) {
        JobHandle handle;
        {
            const std::scoped_lock lock(mutex_);
            handle.value = nextId_++;
            pending_.emplace(handle.value, PendingJob{std::move(job), dependency.value});
            queue_.push_back(handle.value);
        }
        wakeWorkers_.notify_one();
        return handle;
    }

    void workerLoop() {
        for (;;) {
            std::uint64_t jobId = 0;
            Job job;
            {
                std::unique_lock lock(mutex_);
                wakeWorkers_.wait(lock, [&] { return stopping_ || findRunnable(jobId); });
                if (stopping_) {
                    return;
                }
                job = std::move(pending_.at(jobId).job);
                std::erase(queue_, jobId);
            }
            job();
            {
                const std::scoped_lock lock(mutex_);
                pending_.erase(jobId);
            }
            jobDone_.notify_all();
            // A finished dependency may unblock queued jobs.
            wakeWorkers_.notify_all();
        }
    }

    /// Finds a queued job whose dependency (if any) has completed.
    bool findRunnable(std::uint64_t& outJobId) {
        for (const auto id : queue_) {
            const auto& pendingJob = pending_.at(id);
            if (pendingJob.dependency == 0 || !pending_.contains(pendingJob.dependency)) {
                outJobId = id;
                return true;
            }
        }
        return false;
    }

    std::mutex mutex_;
    std::condition_variable wakeWorkers_;
    std::condition_variable jobDone_;
    std::vector<std::thread> workers_;
    std::deque<std::uint64_t> queue_;
    std::unordered_map<std::uint64_t, PendingJob> pending_;
    std::uint64_t nextId_ = 1;
    bool stopping_ = false;
};

} // namespace

std::unique_ptr<IJobScheduler> createThreadPoolScheduler(unsigned threadCount) {
    return std::make_unique<ThreadPoolScheduler>(threadCount);
}

} // namespace sky::core
