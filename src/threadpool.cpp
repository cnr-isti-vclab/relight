#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <future>


struct ThreadPool {
public:
    std::condition_variable finished_task; //notified each time a task finish.

    std::deque<std::packaged_task<void()>> work; //keep track of how many
    std::mutex work_mutex;

private:
    std::vector<std::future<void>> tasks;
    std::condition_variable task_waker;

public:
    ~ThreadPool() {
        finish();
    }

    template<class F, class R=std::result_of_t<F&()>>
    std::future<R> queue(F&& f) {

        std::packaged_task<R()> task(std::forward<F>(f));

        auto result = task.get_future();
        {
            std::unique_lock<std::mutex> l(work_mutex);
            work.emplace_back(std::move(task));
        }
        //start one thread
        task_waker.notify_one();

        return result;
    }

    //initialize threads
    void start(std::size_t n_threads = 1){
        for (std::size_t i = 0; i < n_threads; ++i)
        {
            tasks.push_back( std::async(std::launch::async, [this]{ thread_task(); }) );
        }
    }

    void abort() {
        cancel_pending();
        finish();
    }

    void cancel_pending() {
        std::unique_lock<std::mutex> l(work_mutex);
        work.clear();
    }

    void finish() {
        {
            std::unique_lock<std::mutex> l(work_mutex);
            for(auto&& task: tasks){
                //empty works signals stopping.
                work.push_back({});
            }
        }
        task_waker.notify_all();
        tasks.clear();
    }

private:
    void thread_task() {
        while(true){
            std::packaged_task<void()> task;
            {
                std::unique_lock<std::mutex> lock(work_mutex);
                if (work.empty()){
                    task_waker.wait(lock,[&]{return !work.empty();});
                }
                task = std::move(work.front());
                work.pop_front();
            }
            if (!task.valid()) return;
            task();
            finished_task.notify_one();
        }
    }
};

#endif // THREAD_POOL_H
