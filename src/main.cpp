/**
 * Created by Eleni Alevra
 * modified by Christian Menges
 */

#include <bfs.h>
#include <pagerank.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <algorithm>
#include "thread_pool/thread_pool.h"
#include "thread_pool_pppcsr/thread_pool_pppcsr.h"
#include "utility/sssp.h"

using namespace std;

extern long int doubleTime;
extern long int doubleTime_No_Lock;
extern int one_copy_times;
enum class Operation { READ, ADD, DELETE };
chrono::microseconds totaltime;
// Reads edge list with separator
pair<vector<tuple<Operation, int, int>>, int> read_input(string filename, Operation defaultOp) {
  ifstream f;
  string line;
  f.open(filename);
  if (!f.good()) {
    std::cerr << "Invalid file" << std::endl;
    exit(EXIT_FAILURE);
  }
  vector<tuple<Operation, int, int>> edges;
  int num_nodes = 0;
  std::size_t pos, pos2;
  while (getline(f, line)) {
    int target = stoi(line, &pos);
    int src = stoi(line.substr(pos + 1), &pos2); 
    /* int src = stoi(line, &pos)+1;
    int target = stoi(line.substr(pos + 1), &pos2)+1;  */
    num_nodes = std::max(num_nodes, std::max(src, target));

    Operation op = defaultOp;
    /* if (pos + 1 + pos2 + 1 < line.length()) {
      switch (line[pos + 1 + pos2 + 1]) {
        case '1':
          op = Operation::ADD;
          break;
        case '0':
          op = Operation::DELETE;
          break;
        default:
          cerr << "Invalid operation";
      }
    } */
    op = Operation::ADD;
    edges.emplace_back(op, src, target);
  }
  return make_pair(edges, num_nodes);
}

// Does insertions
template <typename ThreadPool_t>
void update_existing_graph(const vector<tuple<Operation, int, int>> &input, ThreadPool_t *thread_pool, int threads,
                           int size) {
  for (int i = 0; i < size; i++) {
    switch (get<0>(input[i])) {
      case Operation::ADD:
        thread_pool->submit_add(i % threads, get<1>(input[i]), get<2>(input[i]));
        break;
      case Operation::DELETE:
        thread_pool->submit_delete(i % threads, get<1>(input[i]), get<2>(input[i]));
        break;
      case Operation::READ:
        cerr << "Not implemented\n";
        break;
    }
  }
  auto s = chrono::steady_clock::now();
  thread_pool->start(threads);
  thread_pool->stop();
  auto e = chrono::steady_clock::now();
  totaltime+=chrono::duration_cast<chrono::microseconds>(e - s);
}

template <typename ThreadPool_t>
void execute(int threads, int size, const vector<tuple<Operation, int, int>> &core_graph,
             const vector<tuple<Operation, int, int>> &updates, std::unique_ptr<ThreadPool_t> &thread_pool, int batch_size) {
  // Load core graph
  int batches=core_graph.size()/batch_size;
  //auto s = chrono::steady_clock::now();
  for(int i=0;i<batches;i++){
    vector<tuple<Operation, int, int>> batch(core_graph.begin()+i*batch_size,core_graph.begin()+i*batch_size+batch_size);
    update_existing_graph(batch, thread_pool.get(), threads, batch.size());
  }
  //update_existing_graph(core_graph, thread_pool.get(), threads, core_graph.size());
  // Do updates
  //update_existing_graph(updates, thread_pool.get(), threads, size);

  //    DEBUGGING CODE
  //    Check that all edges are there and in sorted order
  //    for (int i = 0; i < core_graph.size(); i++) {
  //        if (!thread_pool->pcsr->edge_exists(std::get<1>(core_graph[i]),std::get<2>(core_graph[i]))) {
  //            cout << "Not there " <<  std::get<1>(core_graph[i]) << " " <<
  //                 std::get<2>(core_graph[i]) << endl;
  //        }
  //    }
  //    for (int i = 0; i < size; i++) {
  //        if (!thread_pool->pcsr->edge_exists(std::get<1>(updates[i]), std::get<2>(updates[i]))) {
  //            cout << "Update not there " << std::get<1>(updates[i]) << " " <<
  //                 std::get<2>(updates[i]) << endl;
  //        }
  //    }
}
static inline int bsr_word(int word) {
  int result;
  __asm__ volatile("bsr %1, %0" : "=r"(result) : "r"(word));
  return result;
}
enum class Version { PPCSR, PPPCSR, PPPCSRNUMA };

int main(int argc, char *argv[]) {
  int threads = 8;
  int size = 1000000;
  int num_nodes = 0;
  bool lock_search = true;
  bool insert = true;
  int pcsr_double_granularity=64;
  Version v = Version::PPPCSRNUMA;
  int partitions_per_domain = 1;
  vector<tuple<Operation, int, int>> core_graph;
  vector<tuple<Operation, int, int>> updates;
  std::string graph_file_name;
  int batch_size=0;
  for (int i = 1; i < argc; i++) {
    string s = string(argv[i]);
    if (s.rfind("-threads=", 0) == 0) {
      threads = stoi(s.substr(string("-threads=").length(), s.length()));
    } else if (s.rfind("-size=", 0) == 0) {
      size = stoi(s.substr(string("-size=").length(), s.length()));
    } else if (s.rfind("-batch_size=", 0) == 0) {
      batch_size = stoi(s.substr(string("-batch_size=").length(), s.length()));
    } else if (s.rfind("-double_granularity=", 0) == 0) {
      pcsr_double_granularity = stoi(s.substr(string("-double_granularity=").length(), s.length()));
    } else if (s.rfind("-lock_free", 0) == 0) {
      lock_search = false;
    } else if (s.rfind("-insert", 0) == 0) {
      insert = true;
    } else if (s.rfind("-delete", 0) == 0) {
      insert = false;
    } else if (s.rfind("-pppcsrnuma", 0) == 0) {
      v = Version::PPPCSRNUMA;
    } else if (s.rfind("-pppcsr", 0) == 0) {
      v = Version::PPPCSR;
    } else if (s.rfind("-ppcsr", 0) == 0) {
      v = Version::PPCSR;
    } else if (s.rfind("-partitions_per_domain=", 0) == 0) {
      partitions_per_domain = stoi(s.substr(string("-partitions_per_domain=").length(), s.length()));
    } else if (s.rfind("-core_graph=", 0) == 0) {
      string core_graph_filename = s.substr(string("-core_graph=").length(), s.length());
      graph_file_name = core_graph_filename;
      int temp = 0;
      std::tie(core_graph, temp) = read_input(core_graph_filename, Operation::ADD);
      num_nodes = std::max(num_nodes, temp);
    } 
    /*
    else if (s.rfind("-update_file=", 0) == 0) {
      string update_filename = s.substr(string("-update_file=").length(), s.length());
      cout << update_filename << endl;
      int temp = 0;
      Operation defaultOp = Operation::ADD;
      if (!insert) {
        defaultOp = Operation::DELETE;
      }
      std::tie(updates, temp) = read_input(update_filename, defaultOp);
      num_nodes = std::max(num_nodes, temp);
      size = std::min((size_t)size, updates.size());
    }*/
  }
  if (core_graph.empty()) {
    cout << "Core graph file not specified" << endl;
    exit(EXIT_FAILURE);
  }
  /*
  if (updates.empty()) {
    cout << "Updates file not specified" << endl;
    exit(EXIT_FAILURE);
  }*/
  //cout << "---Codes start---" << endl;
  //cout << "Core graph nodes: " << num_nodes << endl;
  //cout << "Core graph edges: " << core_graph.size() << endl;
  batch_size=1000000;
  srand(89757);
  random_shuffle(core_graph.begin(), core_graph.end());

  /* int cut_edge_num=0.9*(2 << (bsr_word(core_graph.size())-1));
  core_graph.resize(cut_edge_num); */

  

  switch (v) {
    case Version::PPCSR: {
      auto thread_pool = make_unique<ThreadPool>(threads, lock_search, pcsr_double_granularity, num_nodes + 1, partitions_per_domain);
      execute(threads, size, core_graph, updates, thread_pool,batch_size);
      break;
    }
    case Version::PPPCSR: {
      auto thread_pool =
          make_unique<ThreadPoolPPPCSR>(threads, lock_search, num_nodes + 1, partitions_per_domain, false);
      execute(threads, size, core_graph, updates, thread_pool,batch_size);
      break;
    }
    default: {
      auto thread_pool =
          make_unique<ThreadPoolPPPCSR>(threads, lock_search, num_nodes + 1, partitions_per_domain, true);
      execute(threads, size, core_graph, updates, thread_pool,batch_size);
    }
  }
  cout<<"The total running time is "<<totaltime.count()<<endl;
  cout<<"The doubletime is: "<<doubleTime<<endl;
  cout<< "double time no lock: " << doubleTime_No_Lock << " double time: " << doubleTime << " total time: " << totaltime.count() << " the graph name is: "<< graph_file_name <<std::endl;
  // cout<<" One copy times: " << one_copy_times << std::endl;
  //cout<<"The proportion is "<<(double)doubleTime/totaltime.count()<<endl;
  return 0;
}
