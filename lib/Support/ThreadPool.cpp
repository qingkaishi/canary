/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <unistd.h>
#include <llvm/Support/CommandLine.h>

#include "Support/ThreadPool.h"

using namespace llvm;

static cl::opt<unsigned> NumWorkers("nworkers",
                                    cl::desc("Specify the number of workers to perform analysis. "
                                             "Default is min(the number of hardware cores, 10)."),
                                    cl::value_desc("num of workers"), cl::init(0));


static ThreadPool *Threads = nullptr;

/// hook functions to run at the beginning and end of a thread
/// @{
void (*before_thread_start_hook)() = nullptr;
void (*after_thread_complete_hook)() = nullptr;
/// @}

ThreadPool *ThreadPool::get() {
    if (!Threads)
        Threads = new ThreadPool;
    return Threads;
}

// the constructor just launches the workers
ThreadPool::ThreadPool() : IsStop(false) {
    unsigned NCores = std::thread::hardware_concurrency();
    if (NumWorkers == 0) {
        // We do not fork any threads, just use the main thread
    } else if (NumWorkers > NCores) {
        // Set default value
        NumWorkers.setValue(NCores <= 10 ? (NCores >= 2 ? NCores - 1 : 1) : 10);
    }

    for (auto &Worker : Workers) {
        ThreadLocals[Worker.get_id()] = nullptr;
    }

    NumRunningTask = 0;

    for (unsigned I = 0; I < NumWorkers.getValue(); ++I) {
        Workers.emplace_back([this] {
            if (before_thread_start_hook) before_thread_start_hook();

            for (;;) {
                std::function<void()> Task;

                {
                    std::unique_lock<std::mutex> Lock(this->QueueMutex);
                    this->Condition.wait(Lock, [this] {
                        return this->IsStop || !this->TaskQueue.empty();
                    });
                    // If ThreadPool already stopped, return without checking
                    // tasks.
                    if (this->IsStop) { // && this->tasks.empty())
                        if (after_thread_complete_hook) after_thread_complete_hook();
                        return;
                    }
                    if (!this->TaskQueue.empty()) {
                        Task = std::move(this->TaskQueue.front());
                        this->TaskQueue.pop();
                    }

                    NumRunningTask++;
                }

                Task();

                {
                    std::unique_lock<std::mutex> Lock(this->QueueMutex);
                    NumRunningTask--;
                }
            }
        });
    }
}

void ThreadPool::waitAll() {
    while (true) {
        {
            std::unique_lock<std::mutex> Lock(this->QueueMutex);
            if (TaskQueue.empty() && NumRunningTask == 0) {
                break;
            }
        }
        usleep(10000);
    }
}

ThreadPool::~ThreadPool() { // the destructor shall join all threads
    {
        std::unique_lock<std::mutex> Lock(QueueMutex);
        IsStop = true;
    }
    Condition.notify_all();
    for (std::thread &Worker : Workers) {
        Worker.join();
    }
}
