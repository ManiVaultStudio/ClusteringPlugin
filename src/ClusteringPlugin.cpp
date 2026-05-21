#include "ClusteringPlugin.h"

#include <actions/PluginTriggerAction.h>
#include <util/Icon.h>

#include <PointData/DimensionsPickerAction.h>
#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>

#include <QtCore>

#include <cassert>
#include <vector>

#include <armadillo>
#include <mlpack/methods/dbscan/dbscan.hpp>
#include <mlpack/methods/kmeans/kmeans.hpp>
#include <mlpack/methods/mean_shift/mean_shift.hpp>

Q_PLUGIN_METADATA(IID "studio.manivault.ClusteringPlugin")

using namespace mv::gui;
using namespace mv::plugin;

namespace
{
    // Extract the enabled dimensions from the data
    std::tuple< std::vector<float>, std::vector<std::uint32_t>, size_t, size_t> extractEnabledDimensions(Dataset<Points>& dataset) {
        std::vector<float> data;
        std::vector<std::uint32_t> indices;

        const std::vector<bool> enabledDimensions = dataset->getDimensionsPickerAction().getEnabledDimensions();
        const auto numDimsEnabled = static_cast<size_t>(std::ranges::count_if(enabledDimensions, [](const bool b) { return b; }));
        const size_t numPoints = dataset->getNumPoints();
        const size_t numDimsTotal = dataset->getNumDimensions();

        data.resize(numPoints * numDimsEnabled);

        for (std::uint32_t i = 0; i < numDimsTotal; i++) {
            if (enabledDimensions[i]) {
                indices.push_back(i);
            }
        }

        dataset->populateDataForDimensions(data, indices);

        return { data, indices, numDimsEnabled, numPoints };
    }

}

// =============================================================================
// Plugin
// =============================================================================

ClusteringPlugin::ClusteringPlugin(const PluginFactory* factory) :
    AnalysisPlugin(factory),
    _settingsAction(this)
{
    setObjectName("Clustering Plugin");
}

void ClusteringPlugin::init()
{
    // Create clusters output dataset
    if (!outputDataInit()) {
        setOutputDataset(mv::data().createDataset("Cluster", "Clusters", getInputDataset()));
    }

    auto inputDataset  = getInputDataset<Points>();
    auto outputDataset = getOutputDataset<Clusters>();

    // Add settings to UI    
    auto dimensionsGroupAction = new GroupAction(this, "Dimensions", true);
    dimensionsGroupAction->addAction(&inputDataset->getFullDataset<Points>()->getDimensionsPickerAction());
    dimensionsGroupAction->setText(QString("Input dimensions (%1)").arg(inputDataset->getFullDataset<Points>()->text()));
    dimensionsGroupAction->setShowLabels(false);

    outputDataset->addAction(*dimensionsGroupAction);
    outputDataset->addAction(_settingsAction);

    outputDataset->getDataHierarchyItem().select();

    connect(&_settingsAction.getComputeAction(), &TriggerAction::triggered, this, [this](const bool){
        // Do compute if the settings action is disabled (for instance due to serialization)
        if (!_settingsAction.isEnabled())
            return;

        cluster();
    });

    connect(&_settingsAction.getColorByAction(), &OptionAction::currentIndexChanged, this, [this](const std::int32_t& currentIndex) {
        if (_settingsAction.getUpdateColorsManuallyAction().isChecked())
            return;

        updateClusterColors();
    });

    connect(&_settingsAction.getColorMapAction(), &ColorMapAction::imageChanged, this, [this](const QImage& image) {
        if (_settingsAction.getUpdateColorsManuallyAction().isChecked())
            return;

        updateClusterColors();
    });

    connect(&_settingsAction.getRandomSeedAction(), &IntegralAction::valueChanged, this, [this](const std::int32_t& value) {
        if (_settingsAction.getUpdateColorsManuallyAction().isChecked())
            return;

        updateClusterColors();
    });

    connect(&_settingsAction.getApplyColorsAction(), &TriggerAction::triggered, this, [this](const std::int32_t& value) {
        updateClusterColors();
    });

}

void ClusteringPlugin::updateClusterColors()
{
    switch (static_cast<SettingsAction::ColorBy>(_settingsAction.getColorByAction().getCurrentIndex())) {
    case SettingsAction::ColorBy::PseudoRandomColors:
        Cluster::colorizeClusters(
            getOutputDataset<Clusters>()->getClusters(), 
            _settingsAction.getRandomSeedAction().getValue());
        break;

    case SettingsAction::ColorBy::ColorMap:
        Cluster::colorizeClusters(
            getOutputDataset<Clusters>()->getClusters(), 
            _settingsAction.getColorMapAction().getColorMapImage());
        break;
    }

    events().notifyDatasetDataChanged(getOutputDataset());
}

void ClusteringPlugin::cluster()
{
    auto inputDataset = getInputDataset<Points>();
    auto outputDataset = getOutputDataset<Clusters>();

    // Disable the settings when computing
    _settingsAction.setEnabled(false);

    const QString clusterAlgorithm = _settingsAction.getCurrentClusterAlgorithmName();
    qDebug() << "ClusterAlgorithm:" << clusterAlgorithm;

    auto& task = outputDataset->getTask();
    task.reset();
    task.setProgressMode(mv::Task::ProgressMode::Manual);
    task.setRunning();
    task.setName(clusterAlgorithm);
    task.setProgress(0.1f, "Initializing");
    QCoreApplication::processEvents();

    // data is row-major [pt0_dim0, pt0_dim1, pt1_dim0, pt1_dim1, ...]
    // but armadillo wants colum major so we have to create a copy and transpose
    arma::mat dataset;
    {
        auto [flatData, indices, numDimensions, numPoints] = extractEnabledDimensions(inputDataset);

        if (numDimensions == 0 || numPoints == 0)
        {
            qWarning() << "No dimensions enabled";
            task.setFinished();
            return;
        }

        dataset = arma::conv_to<arma::mat>::from(
            arma::fmat(
                flatData.data(),
                numDimensions,
                numPoints,
                false,  // don't copy memory here, only during conv_to
                true    // strict: prevent reallocation 
            )
        );

    }

    task.setProgress(0.2f, "Clustering");
    QCoreApplication::processEvents();

    // Cluster
    arma::Row<size_t> assignments;  // shape: [1 x numPoints]
    size_t numClusters = 0;

    {
        switch (_settingsAction.getCurrentClusterAlgorithm())
        {
        case SettingsAction::ClusterAlgorithm::KMeans: 
            {
                numClusters = _settingsAction.getKMeansKAction().getValue();
                mlpack::KMeans kmeans;
                kmeans.MaxIterations() = _settingsAction.getNumIterAction().getValue();
                kmeans.Cluster(dataset, numClusters, assignments, false);
                break;
            }
        case SettingsAction::ClusterAlgorithm::MeanShift:
            {
                arma::mat centroids;            // shape: [numDimensions x numClusters]
                const double radiusMultiplier = _settingsAction.getMeanShiftRAction().getValue();
                mlpack::MeanShift ms;

                const double radiusEstimate = ms.EstimateRadius(dataset, 0.2);

                task.setProgress(0.5f, "Clustering");
                QCoreApplication::processEvents();

                ms.Radius(radiusEstimate * radiusMultiplier);
                ms.MaxIterations() = _settingsAction.getNumIterAction().getValue();
                ms.Cluster(dataset, assignments, centroids, false, true);
                numClusters = centroids.n_cols;
                break;
            }
        case SettingsAction::ClusterAlgorithm::DBSCAN:
            {
                const double eps = _settingsAction.getDBSCANepsAction().getValue();
                const std::int32_t minSize = _settingsAction.getDBSCANminSizeAction().getValue();
                mlpack::DBSCAN dbscan(eps, minSize, true);
                dbscan.Cluster(dataset, assignments);

                // points that cannot be associated with any cluster are assigned
                // the id std::numeric_limits<size_t> which we want to replace
                const arma::Row clusterIds = arma::unique(assignments);
                numClusters = clusterIds.n_elem;

                const size_t lastClusterId = (numClusters != 0) ? numClusters - 1 : 0;

                assignments.elem(
                    arma::find(
                        assignments == std::numeric_limits<size_t>::max())
                ).fill(lastClusterId);

                break;
            }
        }
        
        assert(assignments.n_rows == 1);
        assert(assignments.n_cols == inputDataset->getNumPoints());
    }

    task.setProgress(0.8f, "Assigning cluster IDs");
    QCoreApplication::processEvents();

    // Assign cluster IDs to point IDs
    std::vector<std::vector<std::uint32_t>> clusters(numClusters);

    if(numClusters > 0)
    {
        std::vector<std::uint32_t> counts(numClusters, 0);
        for (size_t i = 0; i < assignments.n_elem; ++i) 
            ++counts[assignments[i]];

        for (size_t c = 0; c < numClusters; ++c)
            clusters[c].reserve(counts[c]);

        for (std::uint32_t i = 0; i < assignments.n_elem; ++i)
            clusters[assignments[i]].push_back(i);
    }
    else
    {
        numClusters++;
        auto& singleCluster = clusters.emplace_back();
        singleCluster.resize(assignments.n_cols, 0);
    }

    // Remove existing clusters
    outputDataset->getClusters().clear();

    // Add found clusters
    std::vector<std::uint32_t> globalIndices;
    inputDataset->getSourceDataset<Points>()->getGlobalIndices(globalIndices);

    std::int32_t clusterIndex = 0;
    for (auto& clusterIndicesLocal : clusters)
    {
        Cluster cluster;

        cluster.setName(QString("cluster %1").arg(QString::number(clusterIndex + 1)));

        std::vector<std::uint32_t> clusterIndicesGlobal;

        clusterIndicesGlobal.reserve(clusterIndicesLocal.size());

        for (auto clusterIndexLocal : clusterIndicesLocal)
            clusterIndicesGlobal.push_back(globalIndices[clusterIndexLocal]);

        cluster.setIndices(clusterIndicesGlobal);

        outputDataset->addCluster(cluster);

        clusterIndex++;
    }

    updateClusterColors(); // calls notifyDatasetDataChanged(outputDataset)

    task.setFinished();
    QCoreApplication::processEvents();

    _settingsAction.setEnabled(true);
}

void ClusteringPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    AnalysisPlugin::fromVariantMap(variantMap);

    _settingsAction.fromParentVariantMap(variantMap);
}

QVariantMap ClusteringPlugin::toVariantMap() const
{
    auto variantMap = AnalysisPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

// =============================================================================
// Factory
// =============================================================================

ClusteringPluginFactory::ClusteringPluginFactory()
{
    WidgetAction::setIconByName("circle-nodes");
}

AnalysisPlugin* ClusteringPluginFactory::produce()
{
    return new ClusteringPlugin(this);
}

mv::DataTypes ClusteringPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

PluginTriggerActions ClusteringPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getInstance = [this](const Dataset<Points>& dataset) -> ClusteringPlugin* {
        return dynamic_cast<ClusteringPlugin*>(plugins().requestPlugin(getKind(), Datasets({ dataset })));
    };

    if (datasets.count() == 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {

        auto pluginTriggerAction = new PluginTriggerAction(const_cast<ClusteringPluginFactory*>(this), this, "Clustering", "Apply a clustering algorithm", icon(), [this, getInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (const auto& dataset : datasets)
                getInstance(dataset);
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
