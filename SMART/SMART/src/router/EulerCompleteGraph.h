/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#ifndef EULERCOMPLETEGRAPH_H
#define EULERCOMPLETEGRAPH_H

#include <iostream>
#include <string.h>
#include <algorithm>
#include <list>
#include <vector>
#include <assert.h>
#include <set>
#include <deque>

#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
#include <algorithm>    // std::random_shuffle


// A class that represents an undirected graph
class EulerCompleteGraph
{
    int V;    // No. of vertices
    std::deque<int> *adj;    // A dynamic array of adjacency lists
public:
    // Constructor and destructor
    EulerCompleteGraph(int V)
    {
        this->V = V;
        adj = new std::deque<int>[V];
        for (int i=0; i < V; i++) {
            for (int j=i+1; j<V; j++) {
                this->addEdge(i,j);
            }
        }
    }

    ~EulerCompleteGraph()      { delete [] adj;  }

    // functions to add and remove edge
    void addEdge(int u, int v) {  adj[u].push_back(v); adj[v].push_back(u); }
    void rmvEdge(int u, int v);

    // Methods to print Eulerian tour
    void printEulerTour();
    std::vector<int> printEulerUtil(int s, bool randomSelection=false);

    // This function returns count of vertices reachable from v. It does DFS
    int DFSCount(int v, bool visited[]);

    // Utility function to check if edge u-v is a valid next edge in
    // Eulerian trail or circuit
    bool isValidNextEdge(int u, int v);
};


class EulerBellmanFordGraph
{
public:
    static const int MAX_VALUE = 10000000;
    int V, E;
    std::vector<std::vector < long > > adj;

    EulerBellmanFordGraph(){}
    EulerBellmanFordGraph(int V, int initValue = MAX_VALUE);
    void resetGraphValues(int initValue = MAX_VALUE);
    void setEdgeValue(int src, int dst, long value);
    long getEdgeValue(int src, int dst);

    // The main function that finds shortest distances from src to all other
    // vertices using Bellman-Ford algorithm.
    std::vector< std::deque < int> > BellmanFord(int src, int maxEdges);
};


class EulerPathGenerator {
public:
    static std::set<std::vector<int> > getCoveringPaths(size_t topologySize, int maxHop);

private:
    static std::vector<std::vector<int > > splitEulerTour(const std::vector<int> &eulerPath);

    // a cycle begin and end by the same node
    static std::vector<std::vector<int > > splitCycle(const std::vector<int> &path, size_t maxHop);
    static std::vector<std::vector<int > > addPointToEulerTour(const std::vector<int> &eulerTour, int point);

    // a cycle begin and end by the same node
    static std::vector<std::vector<int > > splitLongPath(const std::vector<int> &path, size_t maxHop);
};


#endif // EULERCOMPLETEGRAPH_H
