/* 
 * File:   hbgraph.hpp
 * Author: jack
 *
 * Created on October 21, 2013, 9:05 PM
 */

#ifndef HBGRAPH_HPP
#define	HBGRAPH_HPP

#include <boost/graph/transitive_closure.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace boost;

typedef adjacency_list<setS,
vecS,
directedS,
property<vertex_index_t, int>
> boostgraph;

class HBGraph {
private:
    boostgraph * hbg;
    boostgraph * TC;
    int vertex_num;

public:

    HBGraph(int size, bool p = false) {
        hbg = new boostgraph(size);
        vertex_num = size;
        TC = NULL;
    }

    ~HBGraph() {
        delete hbg;
        delete TC;
    }

    void insert_edge(int from, int to) {
        if (hbg == NULL) {
            printf("Init graph first!\n");
            exit(-1);
        }

        if (!boost::edge(from, to, *hbg).second)
            add_edge(from, to, *hbg);
        if (TC != NULL) {
            delete TC;
            TC = NULL;
        }
    }

    bool is_reachable(int from, int to) {

        if (TC == NULL) {
            TC = new boostgraph;
            transitive_closure(*hbg, *TC);
        }

        // test reachability
        return boost::edge(from, to, *TC).second;
    }

    int ver_size() {
        return boost::num_vertices(*hbg);
    }
    
    boostgraph* self(){
        return hbg;
    }

};




#endif	/* HBGRAPH_HPP */

