import matplotlib.pyplot as plt
import seaborn as sns; sns.set()
import numpy as np
import random
import sys
from sklearn.neighbors import KernelDensity

def dense_data(n):
    x = []
    for i in range(i, n+1):
        x.append(i)
    return x

def sparse_data(n):
    x = []
    for i in range(i, n+1):
        number = random.randrange(1, n)
        x.append(number)
    return x

def make_data(N, f=0.3, rseed = 1):
    rand = np.random.RandomState(rseed)
    x = rand.randn(N)
    x[int(f*N):] += 5
    return x

data = dense_data(1500)

kde = KernelDensity(bandwidth=1.0, kernel='gaussian')

logprob = kde.score_samples(data[:, None])

plt.fill_between(data, np.exp(logprob), alpha=0.5)
plt.plot(x, np.full_like(x, -0.01), '|k', markeredgewidth=1)
plt.ylim(-0.02, 0.22)

kde = KernelDensity(bandwidth = 1.0, kernel='gaussian')
kde.fit(x[:, None])

logprob = kde.score_samples(x_d[:, None])

plt.fill_between(x_d, np.exp(logprob), alpha=0.5)
plt.plot(x, np.full_like(x, -0.01), '|k',  markeredgewidth=1)
plt.ylim(-0.02, 0.22)
