#include "vectorutils.h"
#include "logger.h"
#include <algorithm>
#include <cmath>

/*
    * Calculate the Pearson correlation coefficient between two vectors.
    * Returns a value between -1 and 1, where 1 means perfect positive correlation,
    * -1 means perfect negative correlation, and 0 means no correlation.
    *
    * The function first checks if the input vectors are valid (non-empty and of the same size).
    * It then calculates the mean of each vector, followed by the covariance and variances.
    * Finally, it computes the Pearson correlation coefficient and clamps it to the range [-1, 1].
    *
    * Math formula:
    * r = (Σ((x_i - mean_x) * (y_i - mean_y))) / (sqrt(Σ((x_i - mean_x)^2)) * sqrt(Σ((y_i - mean_y)^2)))
    *
    * @param vec1 The first input vector.
    * @param vec2 The second input vector.
    * @return The Pearson correlation coefficient between vec1 and vec2.
*/
double VectorUtils::pearsonCorrelation(const std::vector<double>& vec1,
    const std::vector<double>& vec2)
{
    LOG_DEBUG() << "Start: pearsonCorrelation - vec1.size=" << vec1.size()
                << ", vec2.size=" << vec2.size();

    // Validate inputs
    if (vec1.empty() || vec2.empty() || vec1.size() != vec2.size()) {
        LOG_WARNING() << "pearsonCorrelation: Invalid input - vec1 size="
                      << vec1.size() << ", vec2 size=" << vec2.size();
        return 0.0;
    }

    size_t n = vec1.size();

    // Calculate means
    double mean1 = 0.0, mean2 = 0.0;
    for (size_t i = 0; i < n; ++i) {
        mean1 += vec1[i];
        mean2 += vec2[i];
    }
    mean1 /= n;
    mean2 /= n;

    // Calculate covariance and variances
    double covariance = 0.0;
    double variance1 = 0.0;
    double variance2 = 0.0;

    for (size_t i = 0; i < n; ++i) {
        double dev1 = vec1[i] - mean1;
        double dev2 = vec2[i] - mean2;

        covariance += dev1 * dev2;
        variance1 += dev1 * dev1;
        variance2 += dev2 * dev2;
    }

    // Calculate Pearson correlation coefficient
    double correlation = 0.0;
    if (variance1 > 0 && variance2 > 0) {
        correlation = covariance / std::sqrt(variance1 * variance2);
    }

    // Clamp correlation to [-1, 1] (floating point errors can exceed bounds)
    correlation = std::max(-1.0, std::min(1.0, correlation));

    LOG_DEBUG() << "Finish: pearsonCorrelation - correlation=" << correlation;

    return correlation;
}
