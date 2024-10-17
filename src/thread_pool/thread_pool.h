/**
 * Created by Eleni Alevra on 02/06/2020.
 * modified by Christian Menges
 */

#include <queue>
#include <thread>
#include <vector>

#include "../pcsr/PCSR.h"
#include "task.h"

using namespace std;
#ifndef PCSR2_THREAD_POOL_H
#define PCSR2_THREAD_POOL_H
extern int doubleNum;
extern long int doubleTime;
class ThreadPool {
 public:
  PCSR *pcsr;

  explicit ThreadPool(const int NUM_OF_THREADS, bool lock_search, int, uint32_t init_num_nodes, int partitions_per_domain);
  ~ThreadPool() = default;

  /** Public API */
  void submit_add(int thread_id, int src, int dest);     // submit task to thread {thread_id} to insert edge {src, dest}
  void submit_delete(int thread_id, int src, int dest);  // submit task to thread {thread_id} to delete edge {src, dest}
  void submit_read(int, int);  // submit task to thread {thread_id} to read the neighbourhood of vertex {src}
  void start(int threads);     // start the threads
  void stop();                 // stop the threads

 private:
  vector<thread> thread_pool;
  vector<queue<task>> tasks;
  chrono::steady_clock::time_point s;
  chrono::steady_clock::time_point end;
  std::atomic_bool finished;
  struct timeval startT,endT;
  template <bool isMasterThread>
  void execute(int);
};

#endif  // PCSR2_THREAD_POOL_H
