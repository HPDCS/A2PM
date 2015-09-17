/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "EulerCompleteGraph.h"

using namespace std;

/* The main function that print Eulerian Trail. It first finds an odd
   degree vertex (if there is any) and then calls printEulerUtil()
   to print the path */
void EulerCompleteGraph::printEulerTour()
{
    // Find a vertex with odd degree
    int u = 0;
    for (int i = 0; i < V; i++)
        if (adj[i].size() & 1)
        {   u = i; break;  }

    // Print tour starting from oddv
    printEulerUtil(u);
    cout << endl;
}

// random generator function:
static int myrandom (int i) { return std::rand()%i;}


// Print Euler tour starting from vertex u
std::vector<int> EulerCompleteGraph::printEulerUtil(int u, bool randomSelection)
{
    std::vector<int> path;
    path.push_back(u);
    // Recur for all the vertices adjacent to this vertex
    deque<int>::iterator i;

    if (randomSelection){

        std::random_shuffle ( adj[u].begin(), adj[u].end(), myrandom);
//        std::cout << "random_selection" << std::endl;
    }

    for (i = adj[u].begin(); i != adj[u].end(); ++i)
    {
        int v = *i;
        // If edge u-v is not removed and it's a a valid next edge
        if (v != -1 && isValidNextEdge(u, v))
        {
            //cout << u << "-" << v << "  ";
            rmvEdge(u, v);
            std::vector<int> path2=printEulerUtil(v, randomSelection);
            path.insert( path.end(), path2.begin(), path2.end()) ;
        }
    }
    return path;
}

// The function to check if edge u-v can be considered as next edge in
// Euler Tour
bool EulerCompleteGraph::isValidNextEdge(int u, int v)
{
    // The edge u-v is valid in one of the following two cases:

    // 1) If v is the only adjacent vertex of u
    int count = 0;  // To store count of adjacent vertices
    deque<int>::iterator i;
    for (i = adj[u].begin(); i != adj[u].end(); ++i)
        if (*i != -1)
            count++;
    if (count == 1)
        return true;


    // 2) If there are multiple adjacents, then u-v is not a bridge
    // Do following steps to check if u-v is a bridge

    // 2.a) count of vertices reachable from u
    bool visited[V];
    memset(visited, false, V);
    int count1 = DFSCount(u, visited);

    // 2.b) Remove edge (u, v) and after removing the edge, count
    // vertices reachable from u
    rmvEdge(u, v);
    memset(visited, false, V);
    int count2 = DFSCount(u, visited);

    // 2.c) Add the edge back to the graph
    addEdge(u, v);

    // 2.d) If count1 is greater, then edge (u, v) is a bridge
    return (count1 > count2)? false: true;
}

// This function removes edge u-v from graph.  It removes the edge by
// replacing adjcent vertex value with -1.
void EulerCompleteGraph::rmvEdge(int u, int v)
{
    // Find v in adjacency list of u and replace it with -1
    deque<int>::iterator iv = find(adj[u].begin(), adj[u].end(), v);
    *iv = -1;

    // Find u in adjacency list of v and replace it with -1
    deque<int>::iterator iu = find(adj[v].begin(), adj[v].end(), u);
    *iu = -1;
}

// A DFS based function to count reachable vertices from v
int EulerCompleteGraph::DFSCount(int v, bool visited[])
{
    // Mark the current node as visited
    visited[v] = true;
    int count = 1;

    // Recur for all vertices adjacent to this vertex
    deque<int>::iterator i;
    for (i = adj[v].begin(); i != adj[v].end(); ++i)
        if (*i != -1 && !visited[*i])
            count += DFSCount(*i, visited);

    return count;
}

void printPath(const std::vector<int> &path) {
    if (path.size() !=0) {
        std::cout << path[0];
        for (size_t i= 1; i < path.size(); i++) {
            std::cout << "-->" << path[i];
        }
        std::cout << std::endl;

    } else {
        std::cerr << "path Size=0" << std::endl;
    }
}



EulerBellmanFordGraph::EulerBellmanFordGraph(int V, int initValue)
{
    this->V = V;
    adj.resize(V);
    resetGraphValues(initValue);
}

void EulerBellmanFordGraph::resetGraphValues(int initValue) {
    for (int i=0; i < V; i++) {
        adj[i].resize(V);
        for (int j=0; j < V; j++) {

            if (j != i) {
                adj[i][j]=initValue;
            } else {
                adj[i][j]=MAX_VALUE;
            }
        }
    }
}

void EulerBellmanFordGraph::setEdgeValue(int src, int dst, long value) {
    adj[src][dst]=value;
}

long EulerBellmanFordGraph::getEdgeValue(int src, int dst) {
    return adj[src][dst];
}


std::vector< std::deque < int> > EulerBellmanFordGraph::BellmanFord(int src, int maxEdges)
{
    maxEdges = maxEdges + 1;

    std::cout << "Adjacency matrix :" << std::endl;
    for (int u = 0; u < V; u++) {
        for (int v=0; v < V; v++) {
            if (adj[u][v]==MAX_VALUE) {
                std::cout << "--\t";
            } else {
                std::cout << adj[u][v] << "\t";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;

    std::vector< std::vector <int > > dist(maxEdges);
    for(size_t i = 0; i < dist.size(); ++i) {
        dist[i].resize(V);
    }

    std::vector< std::vector <int > > pred(maxEdges);
    for(int i = 0; i < maxEdges; ++i) {
        pred[i].resize(V);
    }

//        std::cout << "bellman step 1" << std::endl;
    // Step 1: Initialize distances from src to all other vertices as INFINITE
    for (int i=0; i < maxEdges; i++) {
        for (int j = 0; j < V; j++) {
            if (j==src) {
                dist[i][j] = 0.0;
            }
            else {
                dist[i][j] = MAX_VALUE;
            }
            pred[i][j] = -1;
        }
    }

//        std::cout << "bellman step 2" << std::endl;
    // Step 2: Relax all edges |V| - 1 times. A simple shortest path from src
    // to any other vertex can have at-most |V| - 1 edges
    for (int i = 1; i < maxEdges ; i++)
    {
        for (int u = 0; u < V; u++) {
            for (int v=0; v < V; v++) {
                if (u != v) {
                    int weight = adj[u][v];
                    if (dist[i][v] > dist[i-1][u] + weight) {
                        dist[i][v] = dist[i-1][u] + weight;
                        pred[i][v] = u;
                    }
                }
            }
        }
    }

//        std::cout << "bellman step 3" << std::endl;
    std::vector< std::deque < int> > shortestPath(V);

    // print resulting paths
    for (int dst=0; dst < V; dst++) {
//            std::cout << "building path for dest=" << dst << std::endl;
        std::deque<int> path;
        if (dst != src) {
            int prev = pred[maxEdges-1][dst];
//                std::cout << "prev[dest]" << prev << std::endl;
            if (prev != -1) {
                path.push_front(dst);
                while(prev != -1) {
                    path.push_front(prev);
                    prev = pred[maxEdges-1][prev];
                }
            }
        }
        shortestPath[dst] = path;
    }

//        std::cout << "bellman print paths" << std::endl;
//        for (int dst=0; dst < V; dst++) {
//            const std::deque<int> & path = shortestPath[dst];
//            if (path.size() != 0) {
//                std::cout << path[0];
//                for (size_t i=1; i< path.size(); i++) {
//                    std::cout << "==>" << path[i];
//                }
//                std::cout<< std::endl;
//            }
//        }
    return shortestPath;
}


std::vector<std::vector<int > > EulerPathGenerator::splitEulerTour(const std::vector<int> &eulerPath) {
    std::vector<std::vector<int > > paths;

    // split each time it pass the source
    std::vector<int > path;
    int s = eulerPath[0];
    path.push_back(s);
    for (size_t i= 1; i < eulerPath.size(); i++) {
        int node=eulerPath[i];
        path.push_back(node);
        if (node==s) {
            paths.push_back(path);
            path.clear();
            path.push_back(s);
        }
    }
    return paths;
}


// a cycle begin and end by the same node
std::vector<std::vector<int > > EulerPathGenerator::splitCycle(const std::vector<int> &path, size_t maxHop){
    std::vector<std::vector<int > > paths;
    std::vector<int> newPath;

    // we verify that it is a cycle, and nonzero
    assert(path[0]==path[path.size()-1] &&  path.size() > 2);

    int s = path[0];
    newPath.push_back(s);

    size_t i= 1;
    //size_t len=0;
    bool done= false;
    while (i < path.size() && !done) {
        int node=path[i];
        if (node!=s) {
            newPath.push_back((node));
            //std::cout << "node non source=" << node << std::endl;
        } else {
            //std::cout << "meeting of the node source =" << node << std::endl;

            paths.push_back(newPath);
            //std::cout << "push new path, because node source met" << std::endl;
            newPath.clear();
            newPath.push_back(s);
            newPath.push_back(path[i-1]);
            paths.push_back(newPath);
            done = true;
        }

        if (newPath.size() >= maxHop+1) {
            paths.push_back(newPath);
            //std::cout << "push new path, because max length reached Out" << std::endl;
            newPath.clear();
            newPath.push_back(s);
            newPath.push_back(node);
        }
        i++;
    }
    return paths;
}

std::vector<std::vector<int > > EulerPathGenerator::addPointToEulerTour(const std::vector<int> &eulerTour, int point) {
    std::vector<std::vector<int > > paths;
    // split each time it pass the source
    std::vector<int > path;
    int s = eulerTour[0];
    path.push_back(s);
    for (size_t i= 1; i < eulerTour.size(); i++) {
        int node=eulerTour[i];
        if (node==s) {
            int lastPoint = path.back();
            path.push_back(point); // addition of point
            path.push_back(path[1]); // back to the second point of the path
            //std::cout << "currentPoint=" << node << ", lastPoint=" << lastPoint << ", adding path : ";
            //printPath(path);
            paths.push_back(path);
            path.clear();
            path.push_back(s);
            path.push_back(lastPoint);
            //std::cout << "adding path : ";
            //printPath(path);
            paths.push_back(path);
            path.clear();
            path.push_back(s);
        } else {
            path.push_back(node);
        }
    }

    // Add the direct path toward the additional point
    path.clear();
    path.push_back(s);
    path.push_back(point);
    paths.push_back(path);
    return paths;
}

// a cycle begins and ends with the same node
std::vector<std::vector<int > > EulerPathGenerator::splitLongPath(const std::vector<int> &path, size_t maxHop){
    std::vector<std::vector<int > > paths;
    std::vector<int> newPath;

    if (path.size()<= maxHop+1) {
        paths.push_back(path);
        return paths;
    }

    int s = path[0];
    newPath.push_back(s);

    size_t i= 1;
    while (i < path.size()) {
        int node=path[i];
        newPath.push_back(node);
        if (newPath.size() == maxHop +1|| i == path.size()-1) {
            paths.push_back(newPath);
            newPath.clear();
            newPath.push_back(s);
            newPath.push_back(node);
        }
        i++;
    }
    return paths;
}

std::set<std::vector<int> > EulerPathGenerator::getCoveringPaths(size_t topologySize, int maxHop) {
    std::set<std::vector<int> > allPaths;

    if (topologySize %2 == 0) {
//            std::cout << "even number of node ==> modification of the Eulerian cycle" << std::endl;
        EulerCompleteGraph gC(topologySize-1);
        std::vector<int> eulerPath= gC.printEulerUtil(0, true);

        // ajout du point au tour eulerien
        //std::cout << "Cutting of the eulerien tour:" << std::endl;
        std::vector<std::vector<int > > paths = addPointToEulerTour(eulerPath, topologySize-1);
        //for (size_t i= 0; i < paths.size(); i++) {
            //std::vector<int> & path = paths[i];
            //printPath(path);
        //}

//            std::cout << "Cutting to the right length:" << std::endl;
        for (size_t i= 0; i < paths.size(); i++) {
            std::vector<std::vector<int > > pathsSplit = splitLongPath(paths[i], maxHop);
            for (size_t j= 0; j < pathsSplit.size(); j++) {
                allPaths.insert(pathsSplit[j]);
            }
        }
    } else {
//            std::cout << "Odd number of node ==> tour eulerien direct" << std::endl;
        EulerCompleteGraph gC(topologySize);
        std::vector<int> eulerPath= gC.printEulerUtil(0, true);

        // cutting of the path
//            std::cout << "Decoupage du tour eulerien :" << std::endl;
        std::vector<std::vector<int > > paths = splitEulerTour(eulerPath);
//            for (size_t i= 0; i < paths.size(); i++) {
//                std::vector<int> & path = paths[i];
//            }

//            std::cout << "cutting of the cycles :" << std::endl;
        for (size_t i= 0; i < paths.size(); i++) {
            std::vector<std::vector<int > > pathsSplit = splitCycle(paths[i], maxHop);
            for (size_t j= 0; j < pathsSplit.size(); j++) {
                allPaths.insert(pathsSplit[j]);
            }
        }
    }
    return allPaths;
}

