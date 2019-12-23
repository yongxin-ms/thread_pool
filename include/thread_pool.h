#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace thread_pool {
template <typename T>
class SafeQueue {
public:
	bool empty() {
		std::unique_lock<std::mutex> lock(mutex_);
		return queue_.empty();
	}

	int size() {
		std::unique_lock<std::mutex> lock(mutex_);
		return queue_.size();
	}

	void enqueue(T& t) {
		std::unique_lock<std::mutex> lock(mutex_);
		queue_.push(t);
	}

	bool dequeue(T& t) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (queue_.empty()) {
			return false;
		}

		t = std::move(queue_.front());
		queue_.pop();
		return true;
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
};

class ThreadPool {
public:
	ThreadPool(int thread_num)
		: thread_num_(thread_num) {}

	void Init() {
		for (int i = 0; i < thread_num_; ++i) {
			auto t = std::make_shared<std::thread>([this]() {
				std::function<void()> func;
				bool dequeued;
				while (!is_shut_down_) {
					do {
						std::unique_lock<std::mutex> lock(conditional_mutex_);
						if (task_queue_.empty()) {
							conditional_lock_.wait(lock);
						}
						dequeued = task_queue_.dequeue(func);
					} while (false);

					if (dequeued) {
						func();
					}
				}
			});

			worker_threads_.push_back(t);
		}
	}

	// Waits until threads finish their current task and shutdowns the pool
	void ShutDown() {
		is_shut_down_ = true;
		conditional_lock_.notify_all();

		for (size_t i = 0; i < worker_threads_.size(); ++i) {
			worker_threads_[i]->join();
		}

		worker_threads_.clear();
		is_shut_down_ = false;
	}

	// Submit a function to be executed asynchronously by the pool
	template <typename F, typename... Args>
	auto submit(F&& f, Args&&... args) {
		// Create a function with bounded parameters ready to execute
		std::function<decltype(f(args...))()> func =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		// Encapsulate it into a shared ptr in order to be able to copy construct / assign
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		// Wrap packaged task into void function
		std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };

		// Enqueue generic wrapper function
		task_queue_.enqueue(wrapper_func);

		// Wake up one thread if its waiting
		conditional_lock_.notify_one();

		// Return future from promise
		return task_ptr->get_future();
	}

private:
	bool is_shut_down_ = false;
	const int thread_num_;
	SafeQueue<std::function<void()>> task_queue_;
	std::vector<std::shared_ptr<std::thread>> worker_threads_;
	std::mutex conditional_mutex_;
	std::condition_variable conditional_lock_;
};
} // namespace thread_pool
