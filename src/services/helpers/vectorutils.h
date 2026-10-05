#ifndef VECTORUTILS_H
#define VECTORUTILS_H

#include <string>
#include <vector>

/**
 * VectorUtils is a utility class that provides static methods for normalizing,
 * resampling, smoothing, and calculating correlations of vectors. These methods
 * are designed to be used in various parts of the application where vector
 * manipulation is required, such as processing audio features or analyzing
 * patterns. The class includes methods for different types of normalization,
 * interpolation techniques, smoothing algorithms, and a method for calculating
 * the Pearson correlation coefficient between two vectors.
 */
class VectorUtils {
public:
    /**
     * @brief Normalizes data so that the minimum value becomes 0 and the maximum
     * value becomes maximum - minimum.
     * @param data Input vector.
     * @param skipZeros If true, zero values are not considered when finding the
     * minimum value.
     * @return Normalized vector.
     */
    static std::vector<double> normalizeByMinMax(const std::vector<double>& data,
        bool skipZeros = true);
    /**
     * @brief Normalizes data so that the minimum value becomes 'from' and the
     * maximum value becomes 'to'.
     * @param from Target minimum value.
     * @param to Target maximum value.
     * @param data Input vector.
     * @return Normalized vector.
     */
    static std::vector<double> normalizeFromTo(double from, double to,
        const std::vector<double>& data);

    /**
     * @brief Normalizes data by scaling the maximum value to 'to'.
     * @param to Target maximum value.
     * @param data Input vector.
     * @return Normalized vector.
     */
    static std::vector<double> normalizeTo(double to,
        const std::vector<double>& data);

    /**
     * @brief Normalizes data by subtracting the mean from each element.
     * @param data Input vector.
     * @param skipZeros If true, zero values are not considered when calculating
     * the mean.
     * @return Normalized vector.
     */
    static std::vector<double> normalizeByMean(const std::vector<double>& data,
        bool skipZeros = true);

    /**
     * @brief Normalizes data by subtracting the mean and dividing by the mean
     * absolute deviation.
     * @param data Input vector.
     * @param skipZeros If true, zero values are not considered when calculating
     * the mean and deviation.
     * @return Normalized vector.
     */
    static std::vector<double>
    normalizeByMeanDeviation(const std::vector<double>& data,
        bool skipZeros = true);

    /**
     * Normalize a 2D vector (matrix) to a range [from, to] using global min/max
     * Finds the global minimum and maximum across all frames, then normalizes all
     * values accordingly
     */
    static std::vector<std::vector<double>>
    normalizeFromTo2D(double from, double to,
        const std::vector<std::vector<double>>& data);

    /**
     * @brief Resamples the input vector to the target length using linear
     * interpolation.
     * @param data Input vector.
     * @param targetLength Target length of the output vector.
     * @return Resampled vector.
     */
    static std::vector<double>
    linearInterpolation(const std::vector<double>& data, int targetLength);

    /**
     * @brief Resamples the input vector to the target length using Linear spline
     * interpolation (Alglib).
     * @param data Input vector.
     * @param targetLength Target length of the output vector.
     * @return Resampled vector.
     */
    static std::vector<double>
    interpolationSplineLinear(const std::vector<double>& data, int targetLength);

    /**
     * @brief Resamples the input vector to the target length using Cubic spline
     * interpolation (Alglib).
     * @param data Input vector.
     * @param targetLength Target length of the output vector.
     * @return Resampled vector.
     */
    static std::vector<double>
    interpolationSplineCubic(const std::vector<double>& data, int targetLength);

    /**
     * @brief Resamples the input vector to the target length using Akima spline
     * interpolation (Alglib).
     * @param data Input vector.
     * @param targetLength Target length of the output vector.
     * @return Resampled vector.
     */
    static std::vector<double>
    interpolationSplineAkima(const std::vector<double>& data, int targetLength);

    /**
     * @brief Resamples the input vector to the target length using Monotone
     * spline interpolation (Alglib).
     * @param data Input vector.
     * @param targetLength Target length of the output vector.
     * @return Resampled vector.
     */
    static std::vector<double>
    interpolationSplineMonotone(const std::vector<double>& data,
        int targetLength);

    /*
     * @brief Interpolates missing frames (zeros) in the input vector.
     * @param type Interpolation type ("Linear", "Hermite", etc.).
     * @param data Input vector.
     * @param skipStartEmptyFrames If true, skips interpolation for leading zeros.
     * @param skipEndEmptyFrames If true, skips interpolation for trailing zeros.
     * @return Vector with interpolated values.
     */
    static std::vector<double> interpolateMissingFrames(const std::string& type,
        const std::vector<double>& data,
        bool skipStartEmptyFrames = false,
        bool skipEndEmptyFrames = false);

    /**
     * @brief Dispatches interpolation based on type string.
     * @param type Interpolation type ("Linear", "Hermite", etc.).
     * @param data Input vector.
     * @param targetLength Target length.
     * @return Resampled vector.
     */
    static std::vector<double> interpolate(const std::string& type,
        const std::vector<double>& data,
        int targetLength);

    /**
     * @brief Smooths data using a Simple Moving Average.
     * @param data Input vector.
     * @param windowSize Size of the smoothing window (must be odd).
     * @return Smoothed vector.
     */
    static std::vector<double>
    smoothMovingAverage(const std::vector<double>& data, int windowSize, bool skipZeros = true);

    /**
     * @brief Smooths data using a Median filter.
     * @param data Input vector.
     * @param windowSize Size of the smoothing window (must be odd).
     * @return Smoothed vector.
     */
    static std::vector<double> smoothMedian(const std::vector<double>& data,
        int windowSize, bool skipZeros = true);

    /**
     * @brief Smooths data using a Gaussian filter.
     * @param data Input vector.
     * @param windowSize Size of the smoothing window (must be odd).
     * @param sigma Standard deviation of the Gaussian kernel.
     * @param skipZeros If true, zero values are kept as-is and excluded from
     * neighbor weighting (treats them as unvoiced frames).
     * @return Smoothed vector.
     */
    static std::vector<double> smoothGaussian(const std::vector<double>& data,
        int windowSize, double sigma, bool skipZeros = true);

    /**
     * @brief Smooths data using Alglib's Penalized Spline.
     * @param data Input vector.
     * @param penalty Penalty coefficient (rho). Large rho = smoother
     * (linear-like), Small rho = closer to data.
     * @return Smoothed vector.
     */
    static std::vector<double> smoothSpline(const std::vector<double>& data,
        double penalty);

    /**
     * @brief General dispatch for smoothing.
     * @param type Smoothing type ("MovingAverage", "Median", "Gaussian",
     * "Spline").
     * @param data Input vector.
     * @param param1 Primary parameter (windowSize or penalty).
     * @param param2 Secondary parameter (polynomialOrder or sigma), if
     * applicable.
     * @param skipZeros If true, zero values are treated as unvoiced frames and
     * excluded from smoothing calculations (e.g., not included in moving average
     * windows and not considered when calculating medians). This helps preserve
     * the integrity of unvoiced frames while still smoothing voiced frames.
     * @return Smoothed vector.
     */
    static std::vector<double> smooth(const std::string& type,
        const std::vector<double>& data,
        double param1, double param2 = 0.0, bool skipZeros = true);

    /**
     * @brief Calculates the Pearson correlation coefficient between two vectors.
     *
     * @param vec1 First vector.
     * @param vec2 Second vector (must have same length as vec1).
     * @return Pearson correlation coefficient in range [-1, 1].
     *         Returns 0.0 if vectors have different sizes or are empty.
     *
     * @details Computes: r = Σ((x-μx)(y-μy)) / √(Σ(x-μx)² * Σ(y-μy)²)
     *          where μx and μy are the means of the vectors.
     *          - r = 1: perfect positive correlation
     *          - r = 0: no correlation
     *          - r = -1: perfect negative correlation
     */
    static double pearsonCorrelation(const std::vector<double>& vec1,
        const std::vector<double>& vec2);
};

#endif // VECTORUTILS_H
