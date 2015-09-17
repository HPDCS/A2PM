/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#ifndef ROUTINGMANAGERCPN_H
#define ROUTINGMANAGERCPN_H

#include "RoutingManager.h"
#include "RandomNeuralNetwork.h"

/**
 * @brief CPN inspired routing algorithm : uses random neural network and reinforcement learning
 *
 *  A random neural network (RNN) is created for each possible destination.
 *  We consider that each neuron represent a possible path to reach the destination.
 *
 *  The algorithm works has follow for reinforcement learning (each time a measurement is received) :
 *      -# Get the reward according to a path p : reward = 1 / G, where G is the measuremnt
 *      -# Get the neuron i corresponding to path p
 *      -# Update the decision threshold according to this formula : T(i) = alpha * T(i) + (1-alpha)* reward
 *      -# Update weigths between neurons depending on if the reward is greater that the threshold or not :
 *          - Delta = |reward - T(i)|
 *          - If reward > T(i) :
 *              - W+(j,i) = W+(j,i) + Delta, for all j
 *              - W-(j,k) = W-(j,k) + Detla/8 , for all j, and for all k!=i
 *          - Else :
 *              - W-(j,i) = W-(j,i) + Delta, for all j
 *              - W+(j,k) = W+(j,k) + Detla/8 , for all j, and for all k!=i
 *      -# Compute firing rate for each neurons : rStar(j) = sum_1^N (W+(j,i) + W-(j,i))
 *      -# Normalize weigths:
 *          - W+(j,k) = W+(j,k) * r(j)/rStar(j)
 *          - W-(j,k) = W-(j,k) * r(j)/rStar(j)
 *      -# Normalize firing rates: r(j) = sum_1^N (W+(j,i) + W-(j,i))
 *      -# Update probability for each neuron j to be excited, by using the following formula, until convergence :
 *          q(j) = min ( 1.0, [ lambda(j)  + sum_{k=1}^{N} q(k) W+(k,j) ] / [ r(j) + lambda (j) + sum{k=1}^N q(k) W-(k,j) ] )
 *
 *  When the algorithm must select the best path, it can select the neuron with the highest q.
 *
 *  For exploration purpose, we choose to draw a set of paths according to the probabilities q.
 *
 */
class RoutingManagerCPN : public RoutingManagerBase
{
public:
    RoutingManagerCPN(size_t maxPathToTest=5, double alpha = 0.7);

    void init();

    virtual void computePathsToTest(std::vector<IPPath> &paths);
    virtual void putProbeResults(const PathEdgesResult &result);
    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() override;

private:
    double alpha; //!< exponential parameter used for updating the thresholds
    bool initialized;
    std::map < IPv4Address , std::vector< IPPath > > allPaths; //!< allPaths[destination] = list of possible paths to reach destination
    std::map < IPPath , size_t > allPathsId; //!< stores the paths indexes
    size_t maxPathToTest; //!< the number of paths that are going to be tested at each monitoring process

    // random neural networks for each source / destination (only for latency QoS for now)
    std::map< IPv4Address,  RandomNeuralNetwork> latencyRNNs; //!< random neural networks for each source / destination (only for latency for now)
    std::map< IPPath, Result > resulTable;  //!< measurement results
};

#endif // ROUTINGMANAGERCPN_H
