import numpy as np
import scipy.stats as stats
import matplotlib.pyplot as plt

if __name__ == '__main__':
    # n = 20
    p = 0.3
    # k = np.arange(0, n + 1)
    #
    # binomial_pmf = stats.binom.pmf(k, n, p)
    # binomial_cdf = stats.binom.cdf(k, n, p)

    for i in range(100, 300, 100):
        binomial_cdf = stats.binom.cdf(np.arange(0, i + 1), i, p)
        plt.plot(np.arange(0, i + 1), binomial_cdf, '*-')
    # plt.plot(k, binomial_pmf, 'o-')
    # plt.plot(k, binomial_cdf, '*-')
    plt.title('distribution', fontsize=15)
    plt.xlabel('number of malicious block')
    plt.ylabel('probability', fontsize=15)
    plt.grid(True)
    plt.show()
