#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>

#include <vector>



struct ThreadPool {
public:
	// Tasks being currently processed
	std::deque<std::packaged_task<void()>> current_tasks;
	// Queue mutex
	std::mutex current_mutex;

private:
	// List of tasks to be processed
	std::vector<std::future<void>> tasks;
	// Condition variable used to notify the thread_task function and tell it to work
	std::condition_variable task_waker;

public:
	~ThreadPool() { finish(); }

	template<class F, class R=std::result_of_t<F&()>> std::future<R> queue(F&& f) {

		std::packaged_task<R()> task(std::forward<F>(f));

		auto result = task.get_future();
		{
			std::unique_lock<std::mutex> l(current_mutex);
			current_tasks.emplace_back(std::move(task));
		}
		//start one thread
		task_waker.notify_one();

		return result;
	}

	void start(std::size_t n_threads = 1){
		nThreads = n_threads;

		for (std::size_t i = 0; i < n_threads; ++i)
			tasks.push_back( std::async(std::launch::async, [this]{ threadTask(); }) );
	}


	void finish() {
		{
			std::unique_lock<std::mutex> l(current_mutex);
			for(auto&& task: tasks) {
				//the thread stops when it encouters an invalid task
				current_tasks.push_back({});
			}
		}
		task_waker.notify_all();
		tasks.clear();
	}
	//unused
	void abort() {
		cancelPending();
		finish();
	}
	//unused
	void cancelPending() {
		std::unique_lock<std::mutex> l(current_mutex);
		current_tasks.clear();
	}

	void waitForSpace() {
		while(true) {
			std::unique_lock<std::mutex> lock(current_mutex);
			if(current_tasks.size() < nThreads)
				break;
		}
	}

private:

	void threadTask() {
		while(true){
			std::packaged_task<void()> task;
			{
				std::unique_lock<std::mutex> lock(current_mutex);
				if (current_tasks.empty()){
					task_waker.wait(lock,[&]{return !current_tasks.empty();});
				}
				task = std::move(current_tasks.front());
				current_tasks.pop_front();
			}
			if (!task.valid()) return;
			task();
		}
	}
private:
	uint32_t nThreads;
};

#endif // THREAD_POOL_H
