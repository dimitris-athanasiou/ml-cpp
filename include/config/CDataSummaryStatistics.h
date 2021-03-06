/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */

#ifndef INCLUDED_ml_config_CDataSummaryStatistics_h
#define INCLUDED_ml_config_CDataSummaryStatistics_h

#include <core/CoreTypes.h>

#include <maths/CBasicStatistics.h>
#include <maths/CBjkstUniqueValues.h>
#include <maths/CCountMinSketch.h>
#include <maths/CEntropySketch.h>
#include <maths/CPRNG.h>
#include <maths/CQuantileSketch.h>
#include <maths/CXMeansOnline1d.h>

#include <config/ImportExport.h>

#include <boost/unordered_map.hpp>

#include <stdint.h>

namespace ml {
namespace config {

//! \brief Basic summary statistics for both categorical and numerical
//! data.
//!
//! DESCRIPTION:\n
//! This gets the time range of the data set and computes the mean rate
//! of records in the data set.
class CONFIG_EXPORT CDataSummaryStatistics {
public:
    CDataSummaryStatistics();

    //! Add an example arriving at \p time.
    void add(core_t::TTime time);

    //! Get the total count of examples.
    uint64_t count() const;

    //! Get the earliest time of any example.
    core_t::TTime earliest() const;

    //! Get the latest time of any example.
    core_t::TTime latest() const;

    //! The mean rate of examples in the data set.
    double meanRate() const;

protected:
    using TMinTimeAccumulator =
        maths::CBasicStatistics::COrderStatisticsStack<core_t::TTime, 1>;
    using TMaxTimeAccumulator =
        maths::CBasicStatistics::COrderStatisticsStack<core_t::TTime, 1, std::greater<core_t::TTime>>;

private:
    //! The earliest example time.
    TMinTimeAccumulator m_Earliest;
    //! The latest example time.
    TMaxTimeAccumulator m_Latest;
    //! The total count of examples.
    uint64_t m_Count;
};

//! \brief Computes simple summary statistics for a categorical data set.
//!
//! DESCRIPTION:\n
//! This computes simple summary statistics of a categorical data set.
//! In particular, it computes count, distinct count, the count of a
//! specified number of the most frequent categories and mean rate.
//!
//! IMPLEMENTATION:\n
//! The estimates of distinct count and most frequent category counts
//! are exact for small data sets and then switch seemlessly to using
//! appropriate sketch data structures for very high distinct counts.
class CONFIG_EXPORT CCategoricalDataSummaryStatistics : public CDataSummaryStatistics {
public:
    using TStrSizePr = std::pair<std::string, std::size_t>;
    using TStrSizePrVec = std::vector<TStrSizePr>;

    //! The smallest cardinality at which we'll approximate the statistics.
    static const std::size_t TO_APPROXIMATE = 5000000;

public:
    explicit CCategoricalDataSummaryStatistics(std::size_t n,
                                               std::size_t toApproximate = TO_APPROXIMATE);
    CCategoricalDataSummaryStatistics(const CDataSummaryStatistics& other,
                                      std::size_t n,
                                      std::size_t toApproximate = TO_APPROXIMATE);

    //! Add an example at \p time.
    void add(core_t::TTime time, const std::string& example);

    //! Get the distinct count of categories.
    std::size_t distinctCount() const;

    //! Get the minimum length of any category.
    std::size_t minimumLength() const;

    //! Get the maximum length of any category.
    std::size_t maximumLength() const;

    //! Get the estimated empirical entropy of the categories.
    double entropy() const;

    //! Get the top-n most frequent categories and their counts.
    void topN(TStrSizePrVec& result) const;

    //! Get the mean count in the remaining categories.
    double meanCountInRemainders() const;

private:
    //! The number of n-grams on which we maintain statistics.
    static const std::size_t NUMBER_N_GRAMS = 5;

private:
    using TUInt32UInt64Pr = std::pair<uint32_t, uint64_t>;
    using TUInt32UInt64PrVec = std::vector<TUInt32UInt64Pr>;
    using TSizeUInt64UMap = boost::unordered_map<std::size_t, uint64_t>;
    using TStrUInt64UMap = boost::unordered_map<std::string, uint64_t>;
    using TStrUInt64UMapItr = TStrUInt64UMap::iterator;
    using TStrUInt64UMapCItr = TStrUInt64UMap::const_iterator;
    using TStrUInt64UMapCItrVec = std::vector<TStrUInt64UMapCItr>;
    using TMinSizeAccumulator =
        maths::CBasicStatistics::COrderStatisticsStack<std::size_t, 1>;
    using TMaxSizeAccumulator =
        maths::CBasicStatistics::COrderStatisticsStack<std::size_t, 1, std::greater<std::size_t>>;
    using TBjkstUniqueValuesVec = std::vector<maths::CBjkstUniqueValues>;
    using TEntropySketchVec = std::vector<maths::CEntropySketch>;

private:
    //! Extract the \p n grams and update the relevant statistics.
    void addNGrams(std::size_t n, const std::string& example);

    //! If the cardinality is too high approximate the statistics.
    void approximateIfCardinalityTooHigh();

    //! Update the counts of the calibrators.
    void updateCalibrators(std::size_t category);

    //! Get the calibrated estimate of the count of \p category.
    double calibratedCount(std::size_t category) const;

    //! Fill in the lowest top-n vector.
    void findLowestTopN();

    //! Get the top-n most frequent categories.
    void topN(TStrUInt64UMapCItrVec& result) const;

private:
    //! A pseudo r.n.g. for deciding whether to sample the n-grams.
    maths::CPRNG::CXorOShiro128Plus m_Rng;

    //! The smallest cardinality at which we'll approximate the statistics.
    std::size_t m_ToApproximate;

    //! Set to true if we are approximating the statistics.
    bool m_Approximating;

    //! The distinct field values and their counts in the data set.
    TSizeUInt64UMap m_ValueCounts;

    //! The approximate distinct count of values.
    maths::CBjkstUniqueValues m_DistinctValues;

    //! A set of exact counts for a small number of categories
    //! which are used to calibrate the count sketch counts.
    TUInt32UInt64PrVec m_Calibrators;

    //! A min-sketch of the category counts.
    maths::CCountMinSketch m_CountSketch;

    //! The number of top-n distinct categories to count.
    std::size_t m_N;

    //! The top n categories by count and their counts.
    TStrUInt64UMap m_TopN;

    //! The smallest count in the top n category counts collection.
    TStrUInt64UMapCItr m_LowestTopN;

    //! The minimum category length.
    TMinSizeAccumulator m_MinLength;

    //! The maximum category length.
    TMaxSizeAccumulator m_MaxLength;

    //! The approximate empirical entropy of the categories.
    maths::CEntropySketch m_EmpiricalEntropy;

    //! The count of distinct n-grams in the categories.
    TBjkstUniqueValuesVec m_DistinctNGrams;

    //! The approximate empirical entropy of the n-grams in the categories.
    TEntropySketchVec m_NGramEmpricalEntropy;
};

//! \brief Computes simple summary statistics of a metric data set.
//!
//! DESCRIPTION:\n
//! Maintains the count of non-numeric values and a fine grained
//! clustering of the data used for estimating the density function
//! of the values.
//!
//! IMPLEMENTATION:\n
//! We use our standard streaming x-means implementation but force
//! it to use only Gaussian modes and allow many more clusters since
//! we want an accurate description of the bulk of the distribution
//! and don't care about over fitting as we do for anomaly detection.
class CONFIG_EXPORT CNumericDataSummaryStatistics : public CDataSummaryStatistics {
public:
    using TDoubleDoublePr = std::pair<double, double>;
    using TDoubleDoublePrVec = std::vector<TDoubleDoublePr>;

public:
    CNumericDataSummaryStatistics(bool integer);
    CNumericDataSummaryStatistics(const CDataSummaryStatistics& other, bool integer);

    //! Add an example at \p time.
    void add(core_t::TTime time, const std::string& example);

    //! Get the minimum value.
    double minimum() const;

    //! Get the approximate median of the values.
    double median() const;

    //! Get the maximum value.
    double maximum() const;

    //! Get a chart of the density function we have estimated.
    bool densityChart(TDoubleDoublePrVec& result) const;

private:
    //! The count of non-numeric values.
    uint64_t m_NonNumericCount;

    //! A quantile sketch used for estimating the median of the
    //! data in a space efficient manner.
    maths::CQuantileSketch m_QuantileSketch;

    //! The principle clusters present in the data.
    maths::CXMeansOnline1d m_Clusters;
};
}
}

#endif // INCLUDED_ml_config_CDataSummaryStatistics_h
