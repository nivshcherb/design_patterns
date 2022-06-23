/* -------------------------------------------------------------------------- */
/* semaphore.hpp                                                              */
/* -------------------------------------------------------------------------- */

#ifndef __DPTOOLS_SEMAPHORE_HPP__
#define __DPTOOLS_SEMAPHORE_HPP__

/* -------------------------------------------------------------------------- */
/* include libraries                                                          */
/* -------------------------------------------------------------------------- */

#include <atomic>               // std::atomic
#include <chrono>               // std::seconds
#include <mutex>                // std::mutex
#include <condition_variable>   // std::conditional_variable

/* -------------------------------------------------------------------------- */
/* import symbols                                                             */
/* -------------------------------------------------------------------------- */

using std::atomic;
using std::mutex;
using std::condition_variable;
using std::chrono::seconds;

/* -------------------------------------------------------------------------- */

class Semaphore
{
public:
    /**
     * @brief Construct a new Semaphore object.
     * @param init_count_ initial value of Semaphore.
     */
    explicit Semaphore(size_t init_count_) noexcept;

    /**
     * @brief Destroy the Semaphore object.
     */
    ~Semaphore() = default;

    // non-copyable
    Semaphore(const Semaphore &) = delete;
    Semaphore& operator=(const Semaphore &) = delete;

    /**
     * @brief Increment the counter of the Semaphore object.
     * Also allows another thread to complete wait().
     * @param n Number to add to the counter. 1 by default.
     */
    void post(size_t n = 1) noexcept;

    /**
     * @brief Decrease the counter of the Semaphore object. This action will
     * block until it is completed.
     */
    void wait() noexcept;

    /**
     * @brief Same as wait(), but without blocking.
     * @return true Decresed Successfully.
     * @return false Didn't decrease.
     */
    bool try_wait() noexcept;

    /**
     * @brief Same as wait(), but only block for a set amount of second;
     * @param timeout_ Maximum blocking time of this action;
     * @return true Decresed Successfully.
     * @return false Didn't decrease.
     */
    bool timed_wait(seconds timeout_) noexcept;

private:
    /**
     * @brief Check if Semaphore is available.
     * @return true Is available
     * @return false Is not available
     */
    bool IsAvailable() const noexcept;

    /* members -------------------------------------------------------------- */
    atomic<size_t>      m_count;        // Number of access available
    mutex               m_lock;         // Critical section lock
    condition_variable  m_condition;    // Access condition
};

/* -------------------------------------------------------------------------- */
#endif /* __DPTOOLS_SEMAPHORE_HPP__ */
