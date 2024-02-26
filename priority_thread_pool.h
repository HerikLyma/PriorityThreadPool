#pragma once

/************************************************************************
 *****************************RUN AS ADMIN*******************************
 ************************************************************************/
#include <span>                // For representing a view over a contiguous sequence
#include <queue>               // For priority queue data structure
#include <atomic>              // For atomic types
#include <thread>              // For managing threads
#include <iostream>            // For standard input/output operations
#include <functional>          // For std::function
#include <syncstream>          // For synchronized output stream
#include <string_view>         // For string_view
#include <shared_mutex>        // For synchronization
#include <condition_variable>  // For condition_variable

#ifdef __linux__ // These values are suggestives and you can change them!
#   include <pthread.h>
#   define THREAD_PRIORITY_LOWEST          99
#   define THREAD_PRIORITY_BELOW_NORMAL    75
#   define THREAD_PRIORITY_NORMAL          50
#   define THREAD_PRIORITY_ABOVE_NORMAL    25
#   define THREAD_PRIORITY_HIGHEST         1
#   define COMPARATOR                      >
#else
#   include <Windows.h>
#   define COMPARATOR                      <
#endif

 // Enumeration definition
enum class Priority : int8_t {
    Lowest = THREAD_PRIORITY_LOWEST,
    Low = THREAD_PRIORITY_BELOW_NORMAL,
    Normal = THREAD_PRIORITY_NORMAL,
    High = THREAD_PRIORITY_ABOVE_NORMAL,
    Realtime = THREAD_PRIORITY_HIGHEST
};

// Operator overloading to print Priority enum values
std::ostream& operator<<(std::ostream& os, Priority priority) {
    switch (priority) {
    case Priority::Lowest:
        os << "Lowest";       // Print priority label for Lowest
        break;
    case Priority::Low:
        os << "Low";          // Print priority label for Low
        break;
    case Priority::Normal:
        os << "Normal";       // Print priority label for Normal
        break;
    case Priority::High:
        os << "High";         // Print priority label for High
        break;
    case Priority::Realtime:
        os << "Realtime";     // Print priority label for Realtime
        break;
    default:
        os << "Unknown";      // Print priority label for unknown values
        break;
    }
    return os;
}


using Task = std::function<void()>;                // Alias for a callable object representing a task
using TaskPriority = std::pair<Task, Priority>;    // Alias for a pair representing a task with its priority

// Comparator for task priorities used in priority queue
struct TaskPriorityComparator {
    [[nodiscard]] bool operator()(const TaskPriority& first, const TaskPriority& second) const {
        return first.second COMPARATOR second.second;
    }
};

// Alias for a priority queue of tasks based on priority
using TasksPriorityQueue = std::priority_queue<TaskPriority, std::vector<TaskPriority>, TaskPriorityComparator>;

class PriorityThreadPool {
public:
    // Deleted move and copy constructors and assignment operators
    PriorityThreadPool(PriorityThreadPool&&) = delete;
    PriorityThreadPool(const PriorityThreadPool&) = delete;
    PriorityThreadPool& operator=(PriorityThreadPool&&) = delete;
    PriorityThreadPool& operator=(const PriorityThreadPool&) = delete;

    // Constructor for PriorityThreadPool
    explicit PriorityThreadPool(const size_t maxThreads = std::thread::hardware_concurrency()) {
        if (maxThreads <= 0) {
            throw std::invalid_argument("maxThreads must be greater than 0!");
        }

        m_threads.reserve(maxThreads);  // Reserve space for threads in the vector

        // Create threads and assign tasks to them
        for (size_t i = 0; i < maxThreads; ++i) {
            m_threads.push_back(std::jthread([this] {
                while (true) {
                    // Locking mutex for thread safety
                    std::unique_lock lock(m_mutex);
                    // Wait until notified or tasks available
                    m_cv.wait(lock, [this] { return m_quit || !m_tasks.empty(); });
                    if (m_tasks.empty()) [[unlikely]] {    // If tasks are empty                    
                        if (m_quit) [[unlikely]] {         // Check if thread pool is quitting                        
                            break;            // Break the loop if quitting
                        }
                        continue;             // Continue to wait for tasks if not quitting
                    }
                    const auto task = m_tasks.top();    // Get the top priority task
                    m_tasks.pop();                      // Remove the task from the queue
                    lock.unlock();                      // Unlock the mutex

                    [[maybe_unused]] const auto priority = static_cast<int>(task.second); // Gets task priority
                    [[maybe_unused]] static constexpr std::string_view errorMessage("Could not change thread priority!\n");
#ifdef __linux__
                    int policy;
                    sched_param param;
                    if (const auto threadId = pthread_self(); pthread_getschedparam(threadId, &policy, &param) == 0) [[likely]] {
                        if (param.sched_priority != priority) [[unlikely]] {
                            policy = SCHED_FIFO;
                            param.sched_priority = priority;
                            if (pthread_setschedparam(threadId, policy, &param) != 0) [[unlikely]] { // If fails                                
                                std::osyncstream(std::cerr) << errorMessage;
                            }
                        }
                    }
#elif _WIN32
                    // Change only if is different
                    if (const auto threadId = GetCurrentThread(); GetThreadPriority(threadId) != priority) [[likely]] {
                        // Change the thread priority on Windows
                        if (!SetThreadPriority(threadId, priority)) [[unlikely]] {  // If fails                           
                            std::osyncstream(std::cerr) << errorMessage;
                        }
                    }
#endif
                    task.first(); // Execute the task
                }
            }));
        }
    }

    // Destructor for PriorityThreadPool
    ~PriorityThreadPool() {
        m_quit = true;       // Set quit flag to true
        m_cv.notify_all();   // Notify all threads to wake up
    }

    // Add a task to the thread pool with specified priority
    void add(const Task task, const Priority priority = Priority::Normal) {
        {
            // Lock mutex for thread safety
            std::lock_guard guard(m_mutex);
            // Add task to the queue
            m_tasks.emplace(std::make_pair(task, priority));
        }
        m_cv.notify_one();
    }

    // Add multiple tasks to the thread pool
    void add(std::span<TaskPriority> tasks) {
        {
            // Lock mutex for thread safety
            std::lock_guard guard(m_mutex);
            // Add each task to the queue
            std::for_each(std::begin(tasks), std::end(tasks), [this](const auto& task) { m_tasks.push(task); });
        }
        m_cv.notify_one();
    }

    // Get the number of remaining tasks in the queue
    [[nodiscard]] size_t remainingTasks() const {
        std::shared_lock guard(m_mutex);   // Lock mutex for read
        return m_tasks.size();             // Return the size of the queue
    }

    // Check if there are remaining tasks in the queue
    [[nodiscard]] bool hasRemainingTasks() const {
        std::shared_lock guard(m_mutex);    // Lock mutex for read
        return !m_tasks.empty();            // Return true if queue is not empty
    }

private:
    std::atomic_bool            m_quit{ false };  // Atomic boolean flag for indicating quitting
    TasksPriorityQueue          m_tasks;          // Priority queue for tasks
    std::vector<std::jthread>   m_threads;        // Vector to hold worker threads
    std::condition_variable_any m_cv;             // Condition variable for synchronization
    mutable std::shared_mutex   m_mutex;          // Mutex for thread safety
};
