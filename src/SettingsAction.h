#pragma once

#include <actions/ColorMap1DAction.h>
#include <actions/DecimalAction.h>
#include <actions/IntegralAction.h>
#include <actions/OptionAction.h>
#include <actions/ToggleAction.h>
#include <actions/TriggerAction.h>

class ClusteringPlugin;

/**
 * Settings class
 *
 */
class SettingsAction : public mv::gui::GroupAction
{
    Q_OBJECT

public:

    /** Cluster options */
    enum class ClusterAlgorithm : std::int32_t {
        KMeans,
        MeanShift,
        DBSCAN
    };

    const QStringList ClusterAlgorithmNames = {
        "KMeans",
        "MeanShift",
        "DBSCAN"
    };

    /** Color options */
    enum class ColorBy : std::uint8_t {
        PseudoRandomColors,     /** Use pseudo-random colors */
        ColorMap                /** Color by continuous color map */
    };

    const QStringList ColorByNames = {
        "Pseudo-random colors",
        "Color map"
    };

public:
    /**
     * Constructor
     * @param clusteringPlugin Pointer to clustering plugin
     */
    SettingsAction(ClusteringPlugin* clusteringPlugin);

public: // Serialization

    /**
     * Load widget action from variant
     * @param variantMap representation of the widget action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save widget action to variant
     * @return Variant representation of the widget action
     */
    QVariantMap toVariantMap() const override;

public: // Action getters

    mv::gui::OptionAction& getColorByAction() { return _colorByAction; }
    mv::gui::ColorMap1DAction& getColorMapAction() { return _colorMapAction; }
    mv::gui::IntegralAction& getRandomSeedAction() { return _randomSeedAction; }
    mv::gui::IntegralAction& getNumIterAction() { return _numIterationsAction; }
    mv::gui::IntegralAction& getKMeansKAction() { return _KMeans_k_Action; }
    mv::gui::DecimalAction& getMeanShiftRAction() { return _MeanShift_r_Action; }
    mv::gui::DecimalAction& getDBSCANepsAction() { return _DBSCAN_eps_Action; }
    mv::gui::IntegralAction& getDBSCANminSizeAction() { return _DBSCAN_minSize_Action; }
    mv::gui::OptionAction& getClusterAlgorithmAction() { return _clusterAlgorithmAction; }
    mv::gui::ToggleAction& getUpdateColorsManuallyAction() { return _updateColorsManuallyAction; }
    mv::gui::TriggerAction& getApplyColorsAction() { return _applyColorsAction; }
    mv::gui::TriggerAction& getComputeAction() { return _computeAction; }

public: // Settings getters
    ClusterAlgorithm getCurrentClusterAlgorithm() const {
        return static_cast<ClusterAlgorithm>(_clusterAlgorithmAction.getCurrentIndex());
    }

    QString getCurrentClusterAlgorithmName() const {
        return ClusterAlgorithmNames[_clusterAlgorithmAction.getCurrentIndex()];
    }

private:
    ClusteringPlugin*                   _clusteringPlugin;              /** Pointer to clustering plugin */
    mv::gui::OptionAction               _colorByAction;                 /** Color by options action */
    mv::gui::ColorMap1DAction           _colorMapAction;                /** Color map action */
    mv::gui::IntegralAction             _randomSeedAction;              /** Random seed action (for coloring) */
    mv::gui::OptionAction               _clusterAlgorithmAction;        /** Cluster algorithms */
    mv::gui::IntegralAction             _numIterationsAction;               /** Multiple: number of iterations */
    mv::gui::IntegralAction             _KMeans_k_Action;               /** KMeans: number of clusters */
    mv::gui::DecimalAction              _MeanShift_r_Action;            /** Mean-Shift: radius multiplier */
    mv::gui::DecimalAction              _DBSCAN_eps_Action;             /** DBSCAN: Radius of each range search */
    mv::gui::IntegralAction             _DBSCAN_minSize_Action;         /** DBSCAN: Minimum number of points for a cluster */
    mv::gui::ToggleAction               _updateColorsManuallyAction;    /** Update colors manually action */
    mv::gui::TriggerAction              _applyColorsAction;             /** Apply colors action */
    mv::gui::TriggerAction              _computeAction;                 /** Compute action */
};
