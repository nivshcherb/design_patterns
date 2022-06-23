/* -------------------------------------------------------------------------- */
/* semaphore.cpp                                                              */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* include libraries                                                          */
/* -------------------------------------------------------------------------- */

#include "semaphore.hpp"

/* -------------------------------------------------------------------------- */
/* aliases                                                                    */
/* -------------------------------------------------------------------------- */

using std::unique_lock;

/* -------------------------------------------------------------------------- */

Semaphore::Semaphore(size_t init_count_) noexcept:
    m_count(init_count_),
    m_lock(),
    m_condition()
{
    // Do nothing
}

void Semaphore::post(size_t n) noexcept
{
    // Critical section begin.
    unique_lock<mutex> guard(m_lock); // Lock guard

    m_count += n;
    m_condition.notify_all();

} // Critical section end.

void Semaphore::wait() noexcept
{
    // Critical section begin
    unique_lock<mutex> guard(m_lock); // Lock guard

    m_condition.wait(guard, [=](){ return IsAvailable(); });
    --m_count;

} // Critical section end

bool Semaphore::try_wait() noexcept
{
    return timed_wait(seconds(0));
}

bool Semaphore::timed_wait(seconds timeout_) noexcept
{
    // Critical section begin
    unique_lock<mutex> guard(m_lock); // Lock guard

    bool result = m_condition.wait_for(guard, timeout_, [=](){ return IsAvailable(); });
    m_count -= result;

    return result;

} // Critical section end

bool Semaphore::IsAvailable() const noexcept
{
    return (m_count > 0);
}

/* -------------------------------------------------------------------------- */