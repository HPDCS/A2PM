#include "RandomNeuralNetwork.h"
#include <cmath>

const double RandomNeuralNetwork::STARTING_Q_VALUE = 0.5;

RandomNeuralNetwork::RandomNeuralNetwork(size_t numberOfNeurons) {

    neurons.resize(numberOfNeurons);
    threshold.resize(numberOfNeurons, 0.0);
    q.resize(numberOfNeurons, STARTING_Q_VALUE);
    Wplus.resize(numberOfNeurons);
    Wminus.resize(numberOfNeurons);
    for (size_t i = 0; i < numberOfNeurons; i++) {
        Wplus[i].resize(numberOfNeurons, 1.0 / (2.0 * (double) (numberOfNeurons-1)));
        Wminus[i].resize(numberOfNeurons, 1.0 / (2.0 * (double) (numberOfNeurons-1)));
        Wplus[i][i] = 0;
        Wminus[i][i] = 0;
    }
}


unsigned int RandomNeuralNetwork::getNumberOfNeurons() const
{
    return neurons.size();
}

void RandomNeuralNetwork::updateNeuronsFiringRates()
{
    // compute firing rate for each neuron
    std::vector<double> rStar(neurons.size(), 0.0);
    for (size_t j = 0; j < neurons.size(); j++) {
        for (size_t i = 0; i < neurons.size(); i++) {
            rStar[j] += Wplus[j][i] + Wminus[j][i];
        }
    }

    // normalize weights
    for (size_t j = 0; j < neurons.size(); j++) {
        double factor = neurons.at(j).r / rStar[j];
        //std::cout << "weigth normalization factor =" << factor << std::endl;
        for (size_t k = 0; k < neurons.size(); k++) {
            //std::cout << "Wplus[" << i << "]["<< j << "]=" << Wplus[i][j];
            //std::cout << "    Wminus[" << i << "]["<< j << "]=" << Wminus[i][j] << std::endl;
            Wplus[j][k] *=  factor;
            Wminus[j][k] *=  factor;
            //std::cout << "Wplus*[" << i << "]["<< j << "]=" << Wplus[i][j];
            //std::cout << "    Wminus*[" << i << "]["<< j << "]=" << Wminus[i][j] << std::endl;
        }
    }
    // recompute neurons firing rates according to normalized weigths
    for (size_t j = 0; j < neurons.size(); j++) {
        //std::cout << "BEFORE : firing rate r[" << j << "]="<< neurons[j].r << std::endl;
        double & firingRate = neurons[j].r;
        firingRate = 0.0;
        for (size_t i = 0; i < neurons.size(); i++) {
            firingRate += Wplus[j][i] + Wminus[j][i];
        }

        //std::cout << "AFTER : firing rate r[" << j << "]="<< neurons[j].r << std::endl;
    }
}

void RandomNeuralNetwork::updateNeuronsProbabilities()
{
    // 2 possibilities for starting values of q before iterating :
    //                1 : keep the latest values
    //                2 : reinit the q to their initial starting value (0.5)
    // According to Lan, the solution implemented in CPN is option 2 (reset to 0.5)
    // According to Pr. Gelenbe, we should keep the latest values, and limit the number of iterations to 10

//    for (size_t i = 0; i< neurons.size(); i++) {
//        q[i] = STARTING_Q_VALUE;
//    }

    std::vector<double> qPrevious = q;
    double sumDeviation = 10000000.0;
    const double CONVERGENCE_THRESHOLD = 0.001;
    const size_t MAX_ITERATION = 10;

    size_t i = 0;

    // check for convergence
    while (sumDeviation >= CONVERGENCE_THRESHOLD && i < MAX_ITERATION) {

        // update q
        for (size_t i = 0; i< neurons.size(); i++) {
            double lambdaPlus = neurons[i].Lambda;
            for (size_t j = 0; j < neurons.size(); j++) {
                lambdaPlus += q[j] * Wplus[j][i];
            }
            double lambdaMinus = neurons[i].lambda;
            for (size_t j = 0; j < neurons.size(); j++) {
                lambdaMinus += q[j] * Wminus[j][i];
            }
            q[i]= std::min(1.0, lambdaPlus / ( neurons[i].r + lambdaMinus));
        }

        // update sum deviation (to check convergence)
        sumDeviation = 0.0;
        for (size_t i = 0; i < neurons.size(); i++) {
            sumDeviation += std::abs(q[i] - qPrevious[i]);
        }

        // update previous q
        qPrevious = q;

        i++;
    }


}
