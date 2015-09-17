#ifndef RANDOMNEURALNETWORK_H
#define RANDOMNEURALNETWORK_H

#include "common/common.h"

/**
 * @brief A random neural network neuron
 */
class RNN_Neuron {
public:
    RNN_Neuron() {
        Lambda = 3.0 / 8.0;
        lambda = 0.0;
        r = 1.0;
    }
    double Lambda; //!< positiveExternalArrivalRate, initial value = 3.0/8.0
    double lambda; //!< negativeExternalArrivalRate, initial value = 0.0
    double r; //!< neuron firing rate, initial value = 1.0
};

/**
 * @brief A simple random neural network (used for reinforcement learning)
 */
class RandomNeuralNetwork
{
public:

    RandomNeuralNetwork() {
    }

    /**
     * @brief Creates a random neural Network
     * @param numberOfNeurons
     */
    RandomNeuralNetwork(size_t numberOfNeurons);
    unsigned int getNumberOfNeurons() const;

    std::vector<RNN_Neuron> neurons; //!< neurons composing the neural network
    std::vector<std::vector<double> > Wplus; //!< matrix of positive weigths W+(i,j), initial values = 1.0 / (2(N-1))
    std::vector<std::vector<double> > Wminus; //!< matrix of negative weigths W-(i,j), , initial values = 1.0 / (2(N-1))

    std::vector<double> threshold; //!< threshold defining if the reward is greater or less than expected, for each neuron
    std::vector<double> q; //!< q[i] = probability that neuron i is excited
    static const double STARTING_Q_VALUE;

    /**
     * @brief Update the neurons firing rates, according to the weigths
     */
    void updateNeuronsFiringRates();

    /**
     * @brief Update neurons probabilities of being excited, according to their firing rate
     * and to the weigths
     */
    void updateNeuronsProbabilities();
};


#endif // RANDOMNEURALNETWORK_H
