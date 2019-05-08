#include "graph.hpp"
#include <list>
#include <algorithm>

graph create_graph(std::istream &in)
{
  using std::vector;

  int prods, shippers, nconns;
  if (!(std::cin >> prods)) throw "failed to get f";
  else if (prods <= 0) throw "non positive f";
  if (!(std::cin >> shippers)) throw "failed to get e";
  else if (shippers <= 0) throw "non positive e";
  if (!(std::cin >> nconns)) throw "failed to get t";
  else if (nconns <= 0) throw "non positive t";


  //  We want minimal cut closer to target so we need
  //  to do Relabel to Front of transposed graph
  graph res(2 + prods + shippers * 2); // source + dest + producers + 2 * shipping
  res.n_producers(prods);
  res.n_shippers(shippers);

  for (ssize_t producer = 0; producer < prods; producer++) {
    int production_value;
    if (!(std::cin >> production_value)) throw "failed to get a producer value";

    // connect producer vertex to dummy sourse with the capacity of the production
    res.add_edge(producer + 2, 0, production_value);
  }

  for (ssize_t s = 0; s < shippers; s++) {
    int max_cap;
    if (!(std::cin >> max_cap)) throw "failed to get a capacity value";

    // split shipper in two; the edge has the capacity
    res.add_edge(s + shippers + prods + 2, s + prods + 2, max_cap);
  }

  for (ssize_t connection = 0; connection < nconns; connection++) {
    int src, dst, cap;
    if (!(std::cin >> src)) throw "failed to get source val";
    if (src < 2) throw "invalid source value";
    if (!(std::cin >> dst)) throw "failed to get destination val";
    if (dst < 1) throw "invalid destination value";
    if (!(std::cin >> cap)) throw "failed to get capacity val";
    if (cap < 1) throw "invalid capacity value";

    // inverted
    res.add_edge_to_shipper(dst, src, cap, prods, shippers);
  }

  return res;
}

int graph::relabel(int u) noexcept // h[u] = 1 + min {h[v] : (u,v) ∈ Ef }
{
  unsigned int min_h = -1;

  node & node_u = _node_list[u];

  for ( auto & edge : node_u.edges() ) {
    node & node_v = _node_list[ edge.dst() ];
    if ( node_u.height() <= node_v.height() && (unsigned) node_v.height() < min_h )
      min_h = node_v.height();
  }

  if (min_h != (unsigned)-1)
    return node_u.relabel(min_h + 1);

  return -1;
}

void graph::push(int u, edge& e) noexcept
{
  int df = std::min( _node_list[u].excess(), e.res_cap() );

  e.add_flow(df);

  _node_list[u].push(df);
  _node_list[e.dst()].recv(df);
}

void graph::initialize_preflow() noexcept
{
  // all heights, excesses and flow already at 0
  //std::cerr << " = initializing preflow\n";
  _node_list[source].relabel(V());
  //std::cerr << " == h[source] = " << V() << ";\n";

  // set iterators
  for ( auto & n : _node_list ) n.current(n.edges().begin());

  for ( auto & e : _node_list[source].edges() ) {
    _node_list[source].recv(e.cap());
    push(source, e);
  }

}

void graph::discharge(int idx) noexcept
{
  node & u = _node_list[idx];
  auto edge_it  = u.current();
  std::cerr << &(*edge_it) << '\n';
  int count = 0;
  while (u.excess() > 0) {
    if (edge_it == u.edges().end()) {
      relabel(idx);
      edge_it = u.edges().begin();
      if (count++ > 2) exit(0);
      continue;
    }

    edge & e = *edge_it;

    std::cerr << " === edge(" << idx << ", " << e.dst()
      << "); cap = " << e.cap()
      << "; flow = " << e.flow()
      << "; residual capacity = " << e.res_cap() << '\n';

    std::cerr << " === h[" << idx << "] = " << _node_list[idx].height() << '\n';
    std::cerr << " === e[" << idx << "] = " << _node_list[idx].excess() << '\n';

    if (e.res_cap() > 0
        && u.height() == _node_list[e.dst()].height() + 1) {
      std::cerr << " === discharging from " << idx << " to " << e.dst() << "\n";
      push(idx, e);
    }
    else {
      std::cerr << " === moving on\n";
      edge_it++;
    }
  }

  u.current(edge_it);
}

void graph::relabel_to_front() noexcept
{
  initialize_preflow();
  std::cerr << " = initialized preflow \n";
  std::list<int> L;
  ssize_t sz = V();
  std::cerr << " = L:";
  for (int i = sz - 1; i > 1; i--) {
    L.push_back(i);
    std::cerr << ' ' << i;
  }
  std::cerr << '\n';


  for (auto it=L.begin(); it != L.end(); ++it) {
    int u = *it;
    int old_h = height(u);
    std::cerr << " = working on " << u
      << "[h = " << old_h
      << "; e = " << _node_list[u].excess() <<"]\n";
    std::cerr << " == started discharge\n";
    discharge(u);
    std::cerr << " == finished discharge\n";
    if (height(u) > old_h) {
      L.push_front(u);
      L.erase(it);
      it = L.begin();
    }
  }
}
