#pragma once

#include <QWidget>
#include "param_card_widget.h"
#include <array>
#include <map>
#include <vector>
#include <string>

class QVBoxLayout;

namespace gis_ai::gui {

class ParamCardWidget;

class ParamWidget : public QWidget {
    Q_OBJECT
public:
    explicit ParamWidget(QWidget* parent = nullptr);

    void setParamSpecs(const std::vector<ParamSpec>& specs);
    std::map<std::string, ParamValue> getParamValues() const;
    void clear();
    bool hasParam(const std::string& key) const;
    void setStringValue(const std::string& key, const std::string& value);
    bool setValueFromString(const std::string& key, const std::string& value);
    void setExtentValue(const std::string& key, const std::array<double, 4>& value);
    std::string stringValue(const std::string& key) const;
    bool validate() const;
    void setUiContext(const std::string& pluginName, const std::string& actionKey);
    void setHighlightedParam(const std::string& key);

signals:
    void paramsChanged();

private:
    void buildCards();
    bool isInputParam(const ParamSpec& spec) const;
    bool isOutputParam(const ParamSpec& spec) const;

    QVBoxLayout* mainLayout_ = nullptr;
    ParamCardWidget* inputCard_ = nullptr;
    ParamCardWidget* outputCard_ = nullptr;
    ParamCardWidget* advancedCard_ = nullptr;

    std::vector<ParamSpec> specs_;
};

}
