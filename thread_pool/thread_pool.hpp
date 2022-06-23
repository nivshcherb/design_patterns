/* -------------------------------------------------------------------------- */
/* thread_pool.hpp                                                            */
/* -------------------------------------------------------------------------- */

#ifndef __ILRD_RD1167_THREAD_POOL_HPP__
#define __ILRD_RD1167_THREAD_POOL_HPP__

/* -------------------------------------------------------------------------- */
/* include libraries                                                          */
/* -------------------------------------------------------------------------- */

#include <condition_variable>   // std::condition_variable
#include <mutex>                // std::mutex, std::unique_lock
#include <stdexcept>            // std::length_error
#include <thread>               // std::thread
#include <unordered_map>        // std::unordered_map
#include <queue>                // std::priority_queue, std::queue

#include <tools/semaphore/semaphore.hpp>

/* -------------------------------------------------------------------------- */
/* import symbols                                                             */
/* -------------------------------------------------------------------------- */

using std::condition_variable;
using std::move;
using std::mutex;
using std::length_error;
using std::unique_lock;
using std::thread;
using std::unordered_map;
using std::priority_queue;
using std::queue;

/* -------------------------------------------------------------------------- */

/**
 * @brief Managing object of set number of threads for execution of Callable
 * objects without the need to constantly create and destroy threads.
 * @tparam Callable Class to execute. Requires operator() with no arguments,
 * and operator<. Also, Callable need to be movable/copyable.
 */
template<class Callable>
class ThreadPool
{
    enum Status { RUNNING, PAUSED, FINISHED };

public:
    /**
     * @brief Construct a new Thread Pool object.
     * @param threads_num_ Number of starting threads. By default, it is the
     * maximum number of threads available, and can not excide this value.
     */
    explicit ThreadPool(size_t threads_num_ = THREAD_MAX);

    /**
     * @brief Destroy the Thread Pool object. 
     */
    ~ThreadPool();

    // non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Pause the Thread Pool's threads from executing untill
     * Continue is called.
     * @return bool Did the action succeed.
     */
    bool Pause();

    /**
     * @brief Resume the Thread Pool's threads after call of Pause().
     * @return bool Did the action succeed.
     */
    bool Continue();

    /**
     * @brief End the Thread Pool and it's threads.
     * @param let_complete_ Should the Thread Pool let all the Callable objects
     * left to be executed.
     * @return bool Did the action succeed.
     */
    bool Finish(bool let_complete_ = true);

    /**
     * @brief Set the number of working threads.
     * 
     * @param nthread_ New number of working threads. By default, it is the
     * maximum number of threads available, and can not excide this value.
     * @return bool Did the action succeed.
     */
    bool SetNumOfThreads(size_t nthread_ = THREAD_MAX);

    /**
     * @brief Add new Callable object to the Thread Pool for
     * it's threads to execute.
     * @param call_ Callable object to execute.
     * @return bool Did the action succeed.
     */
    bool Push(Callable &call_);

    /**
     * @brief Shutdown the Thread Pool and it's threads. This action will block
     * untill all working threads have finished and closed themself.
     * @param complete_calls_ Should this action wait for all calls to end.
     * true by default.Semaphore m_
    /**
     * @brief Return the number of working threads.
     * @return size_t Number of working threads.
     */
    size_t GetSize() const;
    
private:
    static const size_t THREAD_MAX;

    /**
     * @brief Return the first Callable object from Thread Pool.
     * @return Callable Object to execute.
     */
    Callable Pop();

    /**
     * @brief Main loop for threads to run, get Callable object and execute it.
     */
    void ThreadLoop();

    /**
     * @brief Add new threads to Thread pool.
     * @param nthread_ Number of threads to add.
     */
    void AddThreads(size_t nthread_);

    /**
     * @brief Remove threads from Thread pool.
     * @param nthread_ Number of threads to remove.
     */
    void RemoveThreads(size_t nthread_);

    /* members -------------------------------------------------------------- */
    Status          m_status;           // Thread Pool run status.
    unordered_map<thread::id, thread>   // Working threads.
                    m_threads;      
    priority_queue<Callable>            // Callable objects queue.
                    m_calls;
    mutex           m_calls_lock;       // Lock for the queue's actions.
    queue<thread::id>                   // Queue of threads to remove.
                    m_to_remove;
    mutex           m_remove_lock;      // Lock for the remove queue;
    Semaphore       m_is_empty;         // Flag when the Callable queue is empty.
    Semaphore       m_actions;          // Number of actions available.
    Semaphore       m_to_stop;          // Number of threads to stop.
    Semaphore       m_running_threads;  // Number of threads to allow running.
};

/* implementation ----------------------------------------------------------- */

template<class Callable>
const size_t ThreadPool<Callable>::THREAD_MAX = thread::hardware_concurrency;

template<class Callable>
ThreadPool<Callable>::ThreadPool(size_t threads_num_ = THREAD_MAX):
    m_status(Status::RUNNING),
    m_threads(),
    m_calls(),
    m_calls_lock(),
    m_to_remove(),
    m_remove_lock(),
    m_is_empty(0),
    m_actions(0),
    m_to_stop(0),
    m_running_threads(threads_num_)
{
    AddThreads(threads_num_);
}

template<class Callable>
ThreadPool<Callable>::~ThreadPool()
{
    // Finish working threads without completing all Callables.
    Finish(false);
}

template<class Callable>
bool ThreadPool<Callable>::Pause()
{
    // If status is not RUNNING.
    switch (m_status)
    {
        case Status::PAUSED:    return true;
        case Status::FINISHED:  return false;
    }

    // Do not allow any threads to run.
    for (size_t i = 0; i < GetSize(); ++i)
    {
        m_running_threads.wait();
    }

    // Signal threads to recieve pause.
    m_actions.post(GetSize());
}

template<class Callable>
bool ThreadPool<Callable>::Continue()
{
    // If status is not PAUSED
    switch (m_status)
    {
        case Status::RUNNING:   return true;
        case Status::FINISHED:  return false;
    }

    // Allow all threads to run again.
    m_running_threads.post(GetSize());
}

template<class Callable>
bool ThreadPool<Callable>::Finish(bool let_complete_ = true)
{
    if (Status::FINISHED == m_status) return false;

    // Resume in case the Thread Pool was paused.
    Continue();

    // Wait for all Callables to complete if flagged to do so.
    if (let_complete_) m_is_empty.wait();

    m_status = Status::FINISHED;

    // End all working threads.
    RemoveThreads(GetSize());

    return true;
}

template<class Callable>
bool ThreadPool<Callable>::SetNumOfThreads(size_t nthread_ = THREAD_MAX)
{
    if (Status::FINISHED == m_status) return false;
    if (nthread_ > THREAD_MAX) throw(length_error(
        "Number of threads excide the allowed value."
    ));

    if (GetSize() <= nthread_)
    {
        AddThreads(nthread_ - GetSize());
    }
    else
    {
        RemoveThreads(GetSize() - nthread_);
    }

    return true;
}

template<class Callable>
bool ThreadPool<Callable>::Push(Callable &call_)
{
    if (Status::FINISHED == m_status) return false;

    unique_lock<mutex> guard(m_calls_lock); // Critical section start.

    m_calls.push(call_);

    guard.unlock();                         // Critical section end.

    // Signal available Callable
    m_actions.post();

    // Mark Callable queue as not empty
    m_is_empty.try_wait();
} 

template<class Callable>
size_t ThreadPool<Callable>::GetSize() const
{
    return m_threads.size();
}

template<class Callable>
Callable ThreadPool<Callable>::Pop()
{
    unique_lock<mutex> guard(m_calls_lock); // Critical section start.

    Callable ret(move(m_calls.top()));
    m_calls.pop();

}                                           // Critical section end.

template<class Callable>
void ThreadPool<Callable>::ThreadLoop()
{

}

template<class Callable>
void ThreadPool<Callable>::AddThreads(size_t nthread_)
{
    for (size_t i = 0; i < nthread_; ++i)
    {
        thread new_thread(ThreadLoop, this);
        m_threads.insert(new_thread.get_id(), move(new_thread));
    }

    m_running_threads.post(nthread_);
}

template<class Callable>
void ThreadPool<Callable>::RemoveThreads(size_t nthread_)
{

}

/* -------------------------------------------------------------------------- */
#endif /* __ILRD_RD1167_THREAD_POOL_HPP__ */
