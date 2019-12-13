//
// Created by Stefan Richter on 15.05.14.
// Copyright (c) 2014 Stefan Richter. All rights reserved.
//


#ifndef __Zipf_H_
#define __Zipf_H_

#include <math.h>
#include <random>
#include "Types.hpp"

#ifdef STEFAN_ZIPF
class Zipf {
private:
	/**
	* Generates a Zipf distribution according to the code from:
	* <p>
	* Gray, Sundaresan, Englert, Baclawski, Weinberger. "Quickly Generating
	* Billion-Record Synthetic Databases", SIGMOD 1994.
	* <p>
	* Integer k gets weight proportional to (1/k)^theta, 0 &lt theta &lt 1.
	* For theta = 0 -> uniform distribution
	*
	* @author marcos
	**/

    MWord _N;

    double _theta;

    double _zetaN;

    double _zetaTwo;

    std::mt19937_64 _randomEngine;
    std::uniform_real_distribution<double> _distribution;

public:
    /**
     * Instantiates a new ZipfDistributionFromGrayEtAl.
     *
     * @param N
     * @param theta - the skew, 0 &lt theta &lt 1.
     * @param seed
     */
    Zipf(MWord N, double theta, long seed) : _N(N), _theta(theta), _zetaN(zeta(N, theta)), _zetaTwo(zeta(2, theta)), _randomEngine(seed) {

        if (_theta < 0 || _theta >= 1) {
//        if (theta <= 0) {
            throw;
        }
    }

    virtual ~Zipf() {

    }
	
    double zeta(MWord n, double theta) {
        double ans = 0.0;
        for (int i = 1; i <= n; i++) {
            ans += pow(1.0 / i, theta);
        }
        return ans;
    }

    /**
     * Generates the next int in the interval [0, maxRandomValue).
     */
    MWord next() {
        double u = _distribution(_randomEngine);
        double uz = u * _zetaN;

        if (uz < 1.0) {
            return 1;
        }
        if (uz < 1.0 + pow(0.5, _theta)) {
            return 2;
        }
		double alpha = 1.0 / (1.0 - _theta);
        double eta = (1.0 - pow(2.0 / _N, 1.0 - _theta)) / (1.0 - _zetaTwo / _zetaN);
		
        return 1 + (MWord) (_N * pow(eta * u - eta + 1.0, alpha));
    }

    MWord getSize() {
        return _N;
    }
};

#else
 /*************************************************************************
 * This is a simple program to generate a Zipfian probability distribution
 * for a set of N  objects. The Zipf distribution is determined by the
 * skew parameter theta and the number of objects N.
 * The Zipf distribution has the following probability distribution
 * p(i) = c / i ^(1 - theta), where ^ is the exponent operator
 * The skew parameter theta ranges from [0.0, 1.0] with a higher skew for
 * smaller values of theta. A theta of 1.0 results in a uniform distribution
 * To run this program compile using "gcc -o zipf zipf.c -lm"
 **************************************************************************/

#include <stdio.h>
#include <math.h>
#include <cstdlib>


class Zipf {

	struct probvals {
		double prob; /* the access probability */
		double cum_prob; /* the cumulative access probability */
	};

private:

	MWord _max_value;

	std::mt19937_64 _random_engine;
	std::uniform_real_distribution<double> _distribution;

	struct probvals* _zdist; /* the probability distribution  */

	void get_zipf(double theta, MWord N) {

		double sum = 0.0;
		double expo;
		double c = 0.0;
		double sumc = 0.0;
		int i;

		expo = 1 - theta;

		/*
		 * zipfian - p(i) = c / i ^ (1 - theta) At x
		 * = 1, uniform * at x = 0, pure zipfian
		 * This is the N-th harmonic number
		 */

		for (i = 1; i <= N; i++) {
			sum += 1.0 / pow((double) i, expo);
		}
		c = 1.0 / sum;

		for (i = 0; i < N; i++) {
			_zdist[i].prob = c / pow((double) (i + 1), expo);
			sumc += _zdist[i].prob;
			_zdist[i].cum_prob = sumc;
		}

	}

public:
 	Zipf (MWord max_value, double theta, long seed) : _max_value(max_value), _random_engine(seed) {

		if (max_value <= 0 || theta < 0.0 || theta > 1.0) {
			printf("Error in parameters \n");
			exit(1);
		}
		
		_zdist = (struct probvals *) malloc(max_value * sizeof(struct probvals));
		get_zipf(theta, max_value); /* generate the distribution */
	}

	MWord next() {
		double ran = _distribution(_random_engine);
		for (MWord i = 0; i < _max_value; ++i) {
			if (_zdist[i].cum_prob >= ran)
				return i + 1;
		}
	/*	MWord left = 0, right = max_value - 1;
		MWord mid;
		while (left < right) {
			mid = (left + right)/2;
			if (abs(zdist[mid].cum_prob - ran) <= 1)
				break;
			else if (zdist[mid].cum_prob < ran) 
				right = mid - 1;
			else
				left = mid + 1;
		}
		if (zdist[mid].cum_prob < ran) {
			while (zdist[++mid].cum_prob < ran) {}
		} else {
			while (zdist[--mid].cum_prob >= ran) {}
			++mid;
		}
		return mid; */
		return 0;
	}
};

#endif
#endif //__Zipf_H_
