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
#include <mlpack/methods/kmeans/kmeans.hpp>

Q_PLUGIN_METADATA(IID "studio.manivault.ClusteringPlugin")

using namespace mv;
using namespace mv::gui;
using namespace mv::util;
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

        qDebug() << "numPoints:" << numPoints;
        qDebug() << "numDimsTotal:" << numDimsTotal;
        qDebug() << "numDimsEnabled:" << numDimsEnabled;

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
    switch (const auto colorBy = static_cast<SettingsAction::ColorBy>(_settingsAction.getColorByAction().getCurrentIndex())) {
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

    auto& task = outputDataset->getTask();

    // Disable the settings when computing
    _settingsAction.setEnabled(false);

    task.reset();
    task.setRunning();
    task.setName("Clustering " + inputDataset->getLocation());
    task.setProgressDescription("Initializing");

    // data is row-major [pt0_dim0, pt0_dim1, pt1_dim0, pt1_dim1, ...]
    // but armadillo wants colum major so we have to create a copy and transpose
    arma::mat dataset;
    {
        auto [flatData, indices, numDimensions, numPoints] = extractEnabledDimensions(inputDataset);

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

    task.setProgressDescription("Clustering");

    // Cluster
    arma::Row<size_t> assignments;  // shape: [1 x numPoints]
    const size_t numClusters = _settingsAction.getKMeansK().getValue();

    {
        arma::mat centroids;            // shape: [numDimensions x numClusters], not used for now

        mlpack::KMeans kmeans;
        kmeans.Cluster(dataset, numClusters, assignments, centroids);
        assert(assignments.n_elem == inputDataset->getNumPoints());
        assert(centroids.n_rows == inputDataset->getNumDimensions());
        assert(centroids.n_cols == numClusters);
    }

    task.setProgress(0.7f);

    // Assign cluster IDs to point IDs
    std::vector<std::vector<std::uint32_t>> clusters(numClusters);
    {
        std::vector<std::uint32_t> counts(numClusters, 0);
        for (size_t i = 0; i < assignments.n_elem; ++i)
            ++counts[assignments[i]];

        for (size_t c = 0; c < numClusters; ++c)
            clusters[c].reserve(counts[c]);

        for (std::uint32_t i = 0; i < assignments.n_elem; ++i)
            clusters[assignments[i]].push_back(i);
    }

    task.setProgress(0.8f);

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

    updateClusterColors(); // call notifyDatasetDataChanged(outputDataset_

    task.setFinished();

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
