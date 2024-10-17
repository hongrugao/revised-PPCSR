#include <cstdint>
#include <queue>
#include <vector>

using namespace std;

#ifndef PARALLEL_PACKED_CSR_SSSP_H
#define PARALLEL_PACKED_CSR_SSSP_H

template <typename T>
vector<uint32_t> sssp(T &graph, uint32_t start_node) {
  const uint64_t n = graph.get_n();
  vector<uint32_t> out(n, UINT32_MAX);
  vector<bool> visited(n, false);
  out[start_node] = 0;
  visited[start_node] = true;
  bool end_flag=false;
  queue<uint32_t> active_v_queue;
  queue<uint32_t> active_v_queue_next;
  active_v_queue.push(start_node);
  int active_v=0;
  bool v_change[5000000];
  while(!end_flag){
    for(int i = 0; i < n; i++){
        v_change[i] = false;
    }
    while(!active_v_queue.empty()){
        active_v=active_v_queue.front();
        for (const int neighbour : graph.get_neighbourhood(active_v)) {
           int distance=graph.get_distance(active_v,neighbour);
           if (out[active_v]+distance<out[neighbour]) {
            out[neighbour] = out[active_v] + distance;
            if(!v_change[neighbour]) {
                active_v_queue_next.push(neighbour);
                v_change[neighbour] = true;
            }
            
           }
        }
        active_v_queue.pop();
    }
    if(active_v_queue_next.empty()){
        end_flag = true;
    }else{
        while (!active_v_queue_next.empty()){
            active_v_queue.push(active_v_queue_next.front());
            active_v_queue_next.pop();
        }
    }
  }
  return out;
}
/* vector<uint32_t> sssp(T &graph, uint32_t start_node) {
  uint64_t n = graph.get_n();
  vector<uint32_t> out(n, UINT32_MAX);
  vector<bool> visited(n, false);
  out[start_node] = 0;
  visited[start_node] = true;
  
  
  int vtx=start_node;
  for (const int neighbour : graph.get_neighbourhood(vtx)) {
    if (out[neighbour] == UINT32_MAX) {
        out[neighbour] = out[vtx] + 1;
    }
  }
  for(int j=0;j<=n;j++){
    
    if (j % 10000 == 0) cout << "now: " <<  j << endl;
    int temp_distance=UINT32_MAX;
    int next_vtx=start_node;   //找其中一个邻居
    for (int i=0;i<=n;i++) {
      if (out[i]< temp_distance && visited[i]==false) {
        temp_distance=out[i];
        next_vtx=i;
      }
    }
    //cout<<" temp_distance is"<<temp_distance<<endl;
    if(next_vtx==2)
    cout<<" next_vtx is"<<next_vtx<<endl;
    if(next_vtx==start_node){
        break;
    }
    visited[next_vtx]=true;
    for (const int neighbour : graph.get_neighbourhood(next_vtx)) {
      out[neighbour]=min(out[neighbour],out[next_vtx]+1);
    }
  }
  return out;
}
 */
#endif  // PARALLEL_PACKED_CSR_SSSP_H