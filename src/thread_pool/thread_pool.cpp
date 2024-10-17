/**
 * Created by Eleni Alevra on 29/03/2020.
 * modified by Christian Menges
 */

#include "thread_pool.h"

#include <ctime>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <sys/time.h>
using namespace std;
extern int doubleNum; 
extern long int doubleTime;
long int record=0;
extern int countLBalance,countRBalance,distributeTimeOutPage;
/**
 * Initializes a pool of threads. Every thread has its own task queue.
 */
ThreadPool::ThreadPool(const int NUM_OF_THREADS, bool lock_search, int double_granularity, uint32_t init_num_nodes, int partitions_per_domain)
    : finished(false) {
  tasks.resize(NUM_OF_THREADS);
  pcsr = new PCSR(double_granularity, init_num_nodes, init_num_nodes, lock_search, -1);
}

// Function executed by worker threads
// Does insertions, deletions and reads on the PCSR
// Finishes when finished is set to true and there are no outstanding tasks
template <bool isMasterThread>
void ThreadPool::execute(int thread_id) {
  //cout << "Thread " << thread_id << " has " << tasks[thread_id].size() << " tasks" << endl;
  int registered = -1;

  while (!tasks[thread_id].empty() || (!isMasterThread && !finished)) {
    if (!tasks[thread_id].empty()) {
      task t = tasks[thread_id].front();
      tasks[thread_id].pop();

      if (registered == -1) {
        pcsr->edges.global_lock->registerThread();
        registered = 0;
      }
      if (t.add) {
        pcsr->add_edge(t.src, t.target, 1);
      } else if (!t.read) {
        pcsr->remove_edge(t.src, t.target);
      } else {
        pcsr->read_neighbourhood(t.src);
      }

    } else {
      if (registered != -1) {
        pcsr->edges.global_lock->unregisterThread();
        registered = -1;
      }
    }
  }
  if (registered != -1) {
    pcsr->edges.global_lock->unregisterThread();
  }
}

// Submit an update for edge {src, target} to thread with number thread_id
void ThreadPool::submit_add(int thread_id, int src, int target) {
  tasks[thread_id].push(task{true, false, src, target});
}

// Submit a delete edge task for edge {src, target} to thread with number thread_id
void ThreadPool::submit_delete(int thread_id, int src, int target) {
  tasks[thread_id].push(task{false, false, src, target});
}

// Submit a read neighbourhood task for vertex src to thread with number thread_id
void ThreadPool::submit_read(int thread_id, int src) { tasks[thread_id].push(task{false, true, src, src}); }

// starts a new number of threads
// number of threads is passed to the constructor
void ThreadPool::start(int threads) {
  s = chrono::steady_clock::now();
  finished = false;
  gettimeofday(&startT,NULL);
  record=1000000*startT.tv_sec+startT.tv_usec;
  //cout << "The start inserting time: 0" << endl;
  finished = false;
  for (int i = 1 ; i < threads; i++) {
    thread_pool.push_back(thread(&ThreadPool::execute<false>, this, i));
    // Pin thread to core
    //    cpu_set_t cpuset;
    //    CPU_ZERO(&cpuset);
    //    CPU_SET((i * 4), &cpuset);
    //    if (i >= 4) {
    //      CPU_SET(1 + (i * 4), &cpuset);
    //    } else {
    //      CPU_SET(i * 4, &cpuset);
    //    }
    //    int rc = pthread_setaffinity_np(thread_pool.back().native_handle(),
    //                                    sizeof(cpu_set_t), &cpuset);
    //    if (rc != 0) {
    //      cout << "error pinning thread" << endl;
    //    }
  }
  
  execute<true>(0);
}

// Stops currently running worker threads without redistributing worker threads
// start() can still be used after this is called to start a new set of threads operating on the same pcsr
void ThreadPool::stop() {
  finished = true;
  for (auto &&t : thread_pool) {
    if (t.joinable()) t.join();
    //cout << "Done" << endl;
  }
  end = chrono::steady_clock::now();
  gettimeofday(&endT, NULL);
  /* long int buildCoreGraph= 1000000*(endT.tv_sec-startT.tv_sec)+endT.tv_usec-startT.tv_usec;
  printf("the build core graph time = %ld us \n", buildCoreGraph);
  //printf("The total build initial graph time = %ld microseconds \n", buildCoreGraph);
  printf("The double operation time when building initial graph = %ld microseconds \n", doubleTime);
  printf("The double Num when building initial graph = %d \n", doubleNum);
  printf("The proportion is %lf", (double) doubleTime / buildCoreGraph);
  cout << "The total inserting time: " << chrono::duration_cast<chrono::microseconds>(end - s).count() << endl;
  cout << "countR="<<countRBalance<<endl;
  cout << "countL="<<countLBalance<<endl;
  cout << "distributetime="<<distributeTimeOutPage<<endl; */
  thread_pool.clear();
}
