#ifndef _UNIX_SAFE_QUEUE_H
#define _UNIX_SAFE_QUEUE_H
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstddef>
#include <chrono>
#include "error.h"

template <typename T>
struct SafeQueue
{
    SafeQueue() = default;
    ~SafeQueue() = default;

    void push(T value);

    T pop();
    T front();
    bool empty();
    size_t size();
    void wait_wail_empty();
    void wait_wail_empty_for(size_t seconds);

private:
    std::queue<T>         m_queue;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
};

template <typename T>
struct SafeQueue<T *>
{
    SafeQueue() = default;
    ~SafeQueue() = default;

    void push(T *value, std::error_code &ec);
    T *pop();
    T *front();
    bool empty();
    size_t size();
    void wait_wail_empty();
    void wait_wail_empty_for(size_t seconds);

private:
    std::queue<T *>         m_queue;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
};

template <typename T>
void SafeQueue<T *>::push(T *value, std::error_code &ec)
{
    if (value == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    std::unique_lock<std::mutex> ul(m_mutex);
    m_queue.push(value);
    ul.unlock();
    m_cv.notify_one();
}

template <typename T>
T *SafeQueue<T *>::pop()
{
    T * value = nullptr;
    std::unique_lock<std::mutex> ul(m_mutex);

    if (!m_queue.empty())
    {
        value = m_queue.front();
        m_queue.pop();
    }
    return value;
}

template <typename T>
T *SafeQueue<T *>::front()
{
    T * value = nullptr;
    std::lock_guard<std::mutex> lg(m_mutex);

    if (!m_queue.empty())
        value = m_queue.front();

    return value;
}

template <typename T>
bool SafeQueue<T *>::empty()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    bool state = m_queue.empty();
    return state;
}

template <typename T>
size_t SafeQueue<T *>::size()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    size_t size = m_queue.size();
    return size;
}

template <typename T>
void SafeQueue<T *>::wait_wail_empty()
{
    std::unique_lock<std::mutex> ul(m_mutex);
    if (!m_queue.empty())
        return;
    m_cv.wait(ul, [this]{ return !m_queue.empty(); });
}

template <typename T>
void SafeQueue<T *>::wait_wail_empty_for(size_t seconds)
{
    std::unique_lock<std::mutex> ul(m_mutex);
    if (!m_queue.empty())
        return;
    m_cv.wait_for(ul, std::chrono::seconds(seconds), [this]{ return !m_queue.empty(); });
}

template <typename T>
void SafeQueue<T>::push(T value)
{
    std::unique_lock<std::mutex> ul(m_mutex);
    m_queue.push(value);
    ul.unlock();
    m_cv.notify_one();
}

template <typename T>
T SafeQueue<T>::pop()
{
    T value;
    std::unique_lock<std::mutex> ul(m_mutex);

    if (!m_queue.empty())
    {
        value = m_queue.front();
        m_queue.pop();
    }
    return value;
}

template <typename T>
T SafeQueue<T>::front()
{
    T value;
    std::lock_guard<std::mutex> lg(m_mutex);

    if (!m_queue.empty())
        value = m_queue.front();

    return value;
}

template <typename T>
bool SafeQueue<T>::empty()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    bool state = m_queue.empty();
    return state;
}

template <typename T>
size_t SafeQueue<T>::size()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    size_t size = m_queue.size();
    return size;
}

template <typename T>
void SafeQueue<T>::wait_wail_empty()
{
    std::unique_lock<std::mutex> ul(m_mutex);
    if (!m_queue.empty())
        return;
    m_cv.wait(ul, [this]{ return !m_queue.empty(); });
}

template <typename T>
void SafeQueue<T>::wait_wail_empty_for(size_t seconds)
{
    std::unique_lock<std::mutex> ul(m_mutex);
    if (!m_queue.empty())
        return;
    m_cv.wait_for(ul, std::chrono::seconds(seconds), [this]{ return !m_queue.empty(); });
}

#endif
