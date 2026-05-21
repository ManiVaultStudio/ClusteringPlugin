#pragma once

#include <actions/ColorMap1DAction.h>
#include <actions/IntegralAction.h>
#include <actions/OptionAction.h>
#include <actions/ToggleAction.h>
#include <actions/TriggerAction.h>

class ClusteringPlugin;

using namespace mv::gui;

/**
 * Settings class
 *
 */
class SettingsAction : public GroupAction
{
    Q_OBJECT

public:

    /** Color options */
    enum class ColorBy {
        PseudoRandomColors,     /** Use pseudo-random colors */
        ColorMap                /** Color by continuous color map */
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

    OptionAction& getColorByAction() { return _colorByAction; }
    ColorMap1DAction& getColorMapAction() { return _colorMapAction; }
    IntegralAction& getRandomSeedAction() { return _randomSeedAction; }
    IntegralAction& getKMeansK() { return _KMeans_k; }
    ToggleAction& getUpdateColorsManuallyAction() { return _updateColorsManuallyAction; }
    TriggerAction& getApplyColorsAction() { return _applyColorsAction; }
    TriggerAction& getComputeAction() { return _computeAction; }

private:
    ClusteringPlugin*           _clusteringPlugin;              /** Pointer to clustering plugin */
    OptionAction                _colorByAction;                 /** Color by options action */
    ColorMap1DAction            _colorMapAction;                /** Color map action */
    IntegralAction              _randomSeedAction;              /** Random seed action (for coloring) */
    IntegralAction              _KMeans_k;                      /** KMeans: number of clusters */
    ToggleAction                _updateColorsManuallyAction;    /** Update colors manually action */
    TriggerAction               _applyColorsAction;             /** Apply colors action */
    TriggerAction               _computeAction;                 /** Compute action */
};
