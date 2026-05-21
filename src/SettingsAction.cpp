#include "SettingsAction.h"

#include "ClusteringPlugin.h"

using namespace mv::gui;
using namespace mv::util;

SettingsAction::SettingsAction(ClusteringPlugin* clusteringPlugin) :
    GroupAction(clusteringPlugin, "SettingsAction", true),
    _clusteringPlugin(clusteringPlugin),
    _colorByAction(this, "Color by", QStringList({"Pseudo-random colors", "Color map"}), "Color map"),
    _colorMapAction(this, "Color map"),
    _randomSeedAction(this, "Random seed"),
    _KMeans_k(this, "Number of clusters", 1, 250, 25),
    _updateColorsManuallyAction(this, "Update colors manually", false),
    _applyColorsAction(this, "Update colors"),
    _computeAction(this, "Compute")
{
    setText("Clustering settings");

    _randomSeedAction.setUpdateDuringDrag(false);
    _KMeans_k.setUpdateDuringDrag(false);
    _applyColorsAction.setToolTip("Clusters are updated automatically when settings are changed\n(Unless manual updates are toggled on.)");

    GroupAction::addAction(&_colorByAction);
    GroupAction::addAction(&_colorMapAction);
    GroupAction::addAction(&_randomSeedAction);
    GroupAction::addAction(&_KMeans_k);
    GroupAction::addAction(&_updateColorsManuallyAction);
    GroupAction::addAction(&_applyColorsAction);
    GroupAction::addAction(&_computeAction);

    const auto updateReadOnly = [this]() -> void {
        const auto enabled  = !isReadOnly();
        const auto colorBy  = static_cast<ColorBy>(_colorByAction.getCurrentIndex());

        _colorMapAction.setEnabled(enabled && colorBy == ColorBy::ColorMap);
        _randomSeedAction.setEnabled(enabled && colorBy == ColorBy::PseudoRandomColors);
        _KMeans_k.setEnabled(enabled);
        _applyColorsAction.setEnabled(enabled && _updateColorsManuallyAction.isChecked());
    };

    connect(&_colorByAction, &OptionAction::currentIndexChanged, this, [this, updateReadOnly](const std::int32_t& currentIndex) {
        updateReadOnly();
    });

    connect(&_updateColorsManuallyAction, &ToggleAction::toggled, this, [this, updateReadOnly](bool toggled) {
        updateReadOnly();
    });

    updateReadOnly();
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    setEnabled(false);
    {
        _colorByAction.fromParentVariantMap(variantMap);
        _colorMapAction.fromParentVariantMap(variantMap);
        _randomSeedAction.fromParentVariantMap(variantMap);
        _KMeans_k.fromParentVariantMap(variantMap);
        _updateColorsManuallyAction.fromParentVariantMap(variantMap);
        _applyColorsAction.fromParentVariantMap(variantMap);
        _computeAction.fromParentVariantMap(variantMap);
    }
    setEnabled(true);
}

QVariantMap SettingsAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _colorByAction.insertIntoVariantMap(variantMap);
    _colorMapAction.insertIntoVariantMap(variantMap);
    _randomSeedAction.insertIntoVariantMap(variantMap);
    _KMeans_k.insertIntoVariantMap(variantMap);
    _updateColorsManuallyAction.insertIntoVariantMap(variantMap);
    _applyColorsAction.insertIntoVariantMap(variantMap);
    _computeAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
