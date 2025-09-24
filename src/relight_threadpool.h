#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <future>


struct RelightThreadPool {
public:
    // Notified each time a task finishes.
    std::condition_variable finished_task;

    // Queue of task that are currently being processed
    std::deque<std::packaged_task<void()>> work;
    // Mutex used to access the above queue
    std::mutex work_mutex;

private:
    // List of tasks to be processed
    std::vector<std::future<void>> tasks;
    // Condition variable used to notify the thread_task function and tell it to work
    std::condition_variable task_waker;

public:
    RelightThreadPool() {}
    ~RelightThreadPool() { finish(); }

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
        m_MaxThreads = n_threads;

        for (std::size_t i = 0; i < n_threads; ++i)
            tasks.push_back( std::async(std::launch::async, [this]{ thread_task(); }) );
    }

    void abort() {
        cancel_pending();
        finish();
    }

    void cancel_pending() {
        std::unique_lock<std::mutex> l(work_mutex);
        work.clear();
    }

    void waitForSpace() {
        while(true) {
            std::unique_lock<std::mutex> lock(work_mutex);
            if(work.size() < m_MaxThreads)
                break;
            finished_task.wait(lock, [] { return true; });
        }
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
private:
    uint32_t m_MaxThreads;
};

#endif // THREAD_POOL_H
