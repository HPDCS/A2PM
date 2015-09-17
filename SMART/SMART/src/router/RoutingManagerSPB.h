/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#ifndef ROUTINGMANAGERSPB_H
#define ROUTINGMANAGERSPB_H

#include "RoutingManager.h"

class RoutingManagerSPB : public RoutingManagerBase {
public:
    RoutingManagerSPB(size_t maxPathToTest=5, double gamma=0.0, double beta = 0.1, double eta = 1);

    virtual void computePathsToTest(std::vector< IPPath> &pathsToTest) override;

    /**
     * @brief Method called by the path manager when he receive a new latency response
     */
    virtual void putProbeResults(const PathEdgesResult& result);


    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() override;

protected:


    void init();


    double gamma;
    double beta;
    double eta;

    size_t maxPathToTest;
    bool firstRun;
    std::map< IPPath, Result > resulTable;  /**< measurement results */

    bool isInitialized;
    std::map < IPv4Address , std::vector< IPPath > > allPaths;
    std::map < IPPath , size_t > allPathsId;

    std::map<IPv4Address, int> nodesMapIndex;   /**< contains the hops */

    std::map < IPv4Address , std::vector< double > > allPathsProba;
    std::map < IPv4Address , std::vector< double > > allPathsWeigths;
    std::map < IPv4Address, double> allTotalPathsWeigths;

    std::vector< IPPath> C; // covering paths
    std::map< IPv4Address, std::vector<size_t> > inC;
    std::map< IPv4Address, std::vector<size_t> > notInC;
    std::map < IPv4Address, std::vector< size_t> > monitoredPaths;


    class Link{
    public:
        Link() {}
        Link(IPv4Address src, IPv4Address dst): src(src), dst(dst) {}

        friend bool operator<(const Link& l, const Link& r)
        {
            return l.src < r.src && l.dst < r.dst;
        }

        IPv4Address src;
        IPv4Address dst;
    };

    bool linkBelongToPath(const IPPath & path, IPv4Address src, IPv4Address dst);

    std::map<Link, size_t> allLinksId;
    std::vector<Link> allLinks;
    std::map < IPv4Address, std::vector<double> > allLinksWeigths;
    std::map < IPv4Address, std::vector<double> > allLinksProba;
    std::map < IPv4Address, std::map< Link, Result > > latenciesGraph; /**< contains the latencies for each edge*/

};

#endif // ROUTINGMANAGERSPB_H
