#include "SettingsAction.h"

#include <ClusterData/Cluster.h>

#include "ClusteringPlugin.h"

using namespace mv::gui;
using namespace mv::util;

SettingsAction::SettingsAction(ClusteringPlugin* clusteringPlugin) :
    GroupAction(clusteringPlugin, "SettingsAction", true),
    _clusteringPlugin(clusteringPlugin),
    _colorByAction(this, "Color by", ColorByNames, ColorByNames[1]),
    _colorMapAction(this, "Color map"),
    _randomSeedAction(this, "Random seed"),
    _clusterAlgorithmAction(this, "Algorithm", ClusterAlgorithmNames, ClusterAlgorithmNames[0]),
    _numIterationsAction(this, "Iterations", 1, 2000, 500),
    _KMeans_k_Action(this, "Clusters", 1, 250, 25),
    _MeanShift_r_Action(this, "Radius multiplier", 0.1f, 10, 1, 1),
    _DBSCAN_eps_Action(this, "Epsilon", 0.1f, 10, 1, 1),
    _DBSCAN_minSize_Action(this, "Min size", 1, 100, 5),
    _updateColorsManuallyAction(this, "Update colors manually", false),
    _applyColorsAction(this, "Update colors"),
    _computeAction(this, "Compute")
{
    setText("Clustering settings");

    _randomSeedAction.setUpdateDuringDrag(false);
    _KMeans_k_Action.setUpdateDuringDrag(false);
    _MeanShift_r_Action.setUpdateDuringDrag(false);
    _DBSCAN_eps_Action.setUpdateDuringDrag(false);
    _DBSCAN_minSize_Action.setUpdateDuringDrag(false);

    _applyColorsAction.setToolTip("Clusters are updated automatically when settings are changed\n(Unless manual updates are toggled on.)");
    _numIterationsAction.setToolTip("Number of iterations used in cluster computation.");
    _KMeans_k_Action.setToolTip("Number of assumed clusters.");
    _MeanShift_r_Action.setToolTip("A larger `radius multiplier` value will generally result in fewer clusters (e.g. a coarser clustering);\n"
                                   "smaller radius values will generally result in more clusters.");
    _DBSCAN_eps_Action.setToolTip("Radius of each range search.");
    _DBSCAN_minSize_Action.setToolTip("Minimum number of points for a cluster.");

    GroupAction::addAction(&_colorByAction);
    GroupAction::addAction(&_colorMapAction);
    GroupAction::addAction(&_randomSeedAction);
    GroupAction::addAction(&_updateColorsManuallyAction);
    GroupAction::addAction(&_applyColorsAction);

    GroupAction::addAction(&_clusterAlgorithmAction);
    GroupAction::addAction(&_numIterationsAction);
    GroupAction::addAction(&_KMeans_k_Action);
    GroupAction::addAction(&_MeanShift_r_Action);
    GroupAction::addAction(&_DBSCAN_eps_Action);
    GroupAction::addAction(&_DBSCAN_minSize_Action);
    
    GroupAction::addAction(&_computeAction);

    const auto updateReadOnly = [this]() -> void {
        const auto enabled  = !isReadOnly();
        const auto colorBy  = static_cast<ColorBy>(_colorByAction.getCurrentIndex());

        _colorMapAction.setEnabled(enabled && colorBy == ColorBy::ColorMap);
        _randomSeedAction.setEnabled(enabled && colorBy == ColorBy::PseudoRandomColors);
        _numIterationsAction.setEnabled(enabled);
        _KMeans_k_Action.setEnabled(enabled);
        _MeanShift_r_Action.setEnabled(enabled);
        _DBSCAN_eps_Action.setEnabled(enabled);
        _DBSCAN_minSize_Action.setEnabled(enabled);
        _applyColorsAction.setEnabled(enabled && _updateColorsManuallyAction.isChecked());
    };

    const auto updateVisibleSettings = [this](const ClusterAlgorithm clusterAlgorithm) -> void {
        bool kMeans_visible = false;
        bool meanShift_visible = false;
        bool DBSCAN_visible = false;

        switch (clusterAlgorithm) {
        case ClusterAlgorithm::KMeans:
            kMeans_visible = true;
            break;
        case ClusterAlgorithm::MeanShift:
            meanShift_visible = true;
            break;
        case ClusterAlgorithm::DBSCAN:
            DBSCAN_visible = true;
            break;
        }

        _numIterationsAction.setVisible(kMeans_visible || meanShift_visible);
        _KMeans_k_Action.setVisible(kMeans_visible);
        _MeanShift_r_Action.setVisible(meanShift_visible);
        _DBSCAN_eps_Action.setVisible(DBSCAN_visible);
        _DBSCAN_minSize_Action.setVisible(DBSCAN_visible);
    };

    connect(&_colorByAction, &OptionAction::currentIndexChanged, this, [this, updateReadOnly](const std::int32_t& currentIndex) {
        updateReadOnly();
    });

    connect(&_clusterAlgorithmAction, &OptionAction::currentIndexChanged, this, [this, updateVisibleSettings](const std::int32_t& currentIndex) {
        updateVisibleSettings(static_cast<ClusterAlgorithm>(currentIndex));
    });

    connect(&_updateColorsManuallyAction, &ToggleAction::toggled, this, [this, updateReadOnly](bool toggled) {
        updateReadOnly();
    });

    updateReadOnly();
    updateVisibleSettings(static_cast<ClusterAlgorithm>(0));
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    setEnabled(false);

    _colorByAction.fromParentVariantMap(variantMap);
    _colorMapAction.fromParentVariantMap(variantMap);
    _randomSeedAction.fromParentVariantMap(variantMap);
    _numIterationsAction.fromParentVariantMap(variantMap);
    _KMeans_k_Action.fromParentVariantMap(variantMap);
    _MeanShift_r_Action.fromParentVariantMap(variantMap);
    _DBSCAN_eps_Action.fromParentVariantMap(variantMap);
    _DBSCAN_minSize_Action.fromParentVariantMap(variantMap);
    _updateColorsManuallyAction.fromParentVariantMap(variantMap);
    _applyColorsAction.fromParentVariantMap(variantMap);
    _computeAction.fromParentVariantMap(variantMap);

    setEnabled(true);
}

QVariantMap SettingsAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _colorByAction.insertIntoVariantMap(variantMap);
    _colorMapAction.insertIntoVariantMap(variantMap);
    _randomSeedAction.insertIntoVariantMap(variantMap);
    _numIterationsAction.insertIntoVariantMap(variantMap);
    _KMeans_k_Action.insertIntoVariantMap(variantMap);
    _MeanShift_r_Action.insertIntoVariantMap(variantMap);
    _DBSCAN_eps_Action.insertIntoVariantMap(variantMap);
    _DBSCAN_minSize_Action.insertIntoVariantMap(variantMap);
    _updateColorsManuallyAction.insertIntoVariantMap(variantMap);
    _applyColorsAction.insertIntoVariantMap(variantMap);
    _computeAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
