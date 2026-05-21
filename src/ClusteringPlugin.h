#pragma once

#include <AnalysisPlugin.h>
#include <PointData/PointData.h>

#include "SettingsAction.h"

// =============================================================================
// Plugin
// =============================================================================

class ClusteringPlugin : public mv::plugin::AnalysisPlugin
{
    Q_OBJECT

public:
    ClusteringPlugin(const mv::plugin::PluginFactory* factory);

    ~ClusteringPlugin() override = default;
    
    void init() override;

private:
    void cluster();
    void updateClusterColors();

public: // Serialization

    /**
     * Load widget action from variant
     * @param variantMap representation of the widget action
     */
    Q_INVOKABLE void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save widget action to variant
     * @return Variant representation of the widget action
     */
    Q_INVOKABLE QVariantMap toVariantMap() const override;

private:
    SettingsAction          _settingsAction;        /** Settings action */
};

// =============================================================================
// Factory
// =============================================================================

class ClusteringPluginFactory : public mv::plugin::AnalysisPluginFactory
{
    Q_INTERFACES(mv::plugin::AnalysisPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.ClusteringPlugin"
                      FILE  "PluginInfo.json")
    
public:
    ClusteringPluginFactory();

    ~ClusteringPluginFactory() override = default;

    mv::plugin::AnalysisPlugin* produce() override;

    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
