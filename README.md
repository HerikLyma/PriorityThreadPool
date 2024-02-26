# PriorityThreadPool

**PriorityThreadPool** is a C++ implementation of a thread pool that allows the execution of tasks with different priorities. It is designed to be easily integrated into C++ projects and offers the ability to specify the priority of each task added to the thread pool.

## Advantages

- **Priority Management**: Enables the execution of tasks with different priority levels, which is useful in scenarios where certain tasks are more critical than others.
- **Task Reordering**: Tasks added with higher priorities may be moved to the front of the internal queue of the thread pool and have their execution priority modified.
- **Easy Integration**: It consists of just a header file (`priority_thread_pool.h`), making inclusion in any C++ project straightforward.
- **Cross-Platform**: Supports both Windows and Linux operating systems.
- **Ease of Use**: Provides a simple interface for adding tasks and checking the status of the thread pool.

## Usage

Here's a simple example of how to use `PriorityThreadPool`:

```cpp
#include "priority_thread_pool.h"

void task1() {
    // Implement your task here
}

void task2() {
    // Implement your task here
}

int main() {
    PriorityThreadPool threadPool(1);

    // Add tasks with different priorities
    threadPool.add(task1, Priority::High);
    threadPool.add(task2, Priority::Lowest);

    return 0;
}
```

This example creates a `PriorityThreadPool`, adds two tasks with different priorities, and exits after all tasks are completed.

## Advanced Example

```cpp
#include "priority_thread_pool.h"

// Function to generate tasks with specified priority and add them to a vector
void generateTasks(const Priority priority, const int tasks, std::vector<TaskPriority>& out) {
    for (int i = 0; i < tasks; ++i) {
        Task task = [priority] {
                std::osyncstream(std::cout) << "Executing " << priority << " priority task!" << std::endl;
            };
        // Add task to the vector
        out.push_back(std::make_pair(std::move(task), priority));
    }
}

int main() {
    // Specify the number of worker threads
    const auto workerThreads = 1;
    // Create a thread pool with the specified number of threads
    PriorityThreadPool pool(workerThreads);
    // Vector to hold tasks and their priorities
    std::vector<TaskPriority> tasks;
    // Generate tasks with different priorities
    generateTasks(Priority::Normal, 8, tasks);
    generateTasks(Priority::Lowest, 1, tasks);
    generateTasks(Priority::High, 2, tasks);
    generateTasks(Priority::Low, 4, tasks);
    generateTasks(Priority::Realtime, 1, tasks);
    // Add all tasks to the thread pool
    pool.add(tasks);

    return 0;
}
```

In this example, multiple tasks are added to the `PriorityThreadPool` with different priorities. Each task prints a message to indicate its execution. Tasks with higher priorities may be executed before tasks with lower priorities, and the execution priority can be modified to ensure priority behavior as needed.

## Benchmark Example

```cpp
#include <chrono>
#include <cassert>
#include "priority_thread_pool.h"

std::atomic_uint64_t counter;
const uint64_t NUM_TASKS = 1000 * std::thread::hardware_concurrency(); // Total number of tasks to be executed

void incCounter() {
    ++counter;
}

// Generic function to run a test
template<typename Function>
void runTest(const std::string& testName, Function&& func) {
    counter = 0;
    std::cout << "Testing: " << testName << std::endl;
    std::cout << "Total tasks: " << NUM_TASKS << std::endl;
    const auto start = std::chrono::steady_clock::now();
    func();
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Time taken: " << elapsed << " milliseconds\n" << std::endl;
    assert(counter == NUM_TASKS);
}

// Function to compare the performance with PriorityThreadPool and without PriorityThreadPool
void comparePerformance() {
    // Test with PriorityThreadPool
    runTest("With PriorityThreadPool", [] {
        PriorityThreadPool threadPool;
        for (int i = 0; i < NUM_TASKS; ++i) {
            threadPool.add(incCounter);
        }
    });

    // Test with thread creation and destruction all the time, as here we don't have refined control
    // over how many threads we are creating, as there is no pool!
    runTest("Without PriorityThreadPool", [] {
        std::vector<std::jthread> threads;
        threads.reserve(NUM_TASKS);
        for (int i = 0; i < NUM_TASKS; ++i) {
            threads.emplace_back(incCounter);
        }
    });
}

int main() {
    comparePerformance();
    return 0;
}
```

This example performs a benchmark comparing the performance of executing a large number of tasks with and without `PriorityThreadPool`.
