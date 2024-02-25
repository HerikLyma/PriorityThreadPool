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
#include <iostream>

void printTask(int id) {
    std::cout << "Task " << id << " is running\n";
}

int main() {
    PriorityThreadPool threadPool(1);

    // Add multiple tasks with different priorities
    for (int i = 0; i < 10; ++i) {
        threadPool.add([i]() { printTask(i); }, Priority::Normal);
    }
    threadPool.add([]() { std::cout << "Critical task running\n"; }, Priority::High);
    threadPool.add([]() { std::cout << "Low priority task running\n"; }, Priority::Lowest);

    return 0;
}
```

In this example, multiple tasks are added to the `PriorityThreadPool` with different priorities. Each task prints a message to indicate its execution. Tasks with higher priorities may be executed before tasks with lower priorities, and the execution priority can be modified to ensure priority behavior as needed.
