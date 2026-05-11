#pragma once

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMap>
#include <memory>
#include <string>
#include <variant>
#include <array>
#include <vector>

enum class ParamType {
    String, Int, Double, Bool, FilePath, DirPath, Enum, Extent, CRS
};

using ParamValue = std::variant<std::string, int, double, bool, std::vector<std::string>, std::array<double, 4>>;

struct ParamSpec {
    std::string key;
    std::string displayName;
    std::string description;
    ParamType type = ParamType::String;
    bool required = true;
    ParamValue defaultValue = std::string{};
    ParamValue minValue = int{0};
    ParamValue maxValue = int{0};
    std::vector<std::string> enumValues;
};

class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QPushButton;

struct ParamWidgetEntry {
    std::string key;
    QWidget* widget = nullptr;
    QLineEdit* lineEdit = nullptr;
    QComboBox* comboBox = nullptr;
    QDoubleSpinBox* spinBox = nullptr;
    QSpinBox* intSpinBox = nullptr;
    QCheckBox* checkBox = nullptr;
    QPushButton* browseButton = nullptr;
    QPushButton* auxButton = nullptr;
};

namespace gis_ai::gui {

class ParamCardWidget : public QWidget {
    Q_OBJECT
public:
    enum class CardType { Input, Output, Advanced };

    explicit ParamCardWidget(CardType type, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void addParam(const ParamSpec& spec);
    void clearParams();

    QMap<std::string, ParamValue> collectValues() const;
    bool validate() const;
    bool hasParam(const std::string& key) const;
    void setStringValue(const std::string& key, const std::string& value);
    bool setValueFromString(const std::string& key, const std::string& value);
    void setExtentValue(const std::string& key, const std::array<double, 4>& value);

    void markFieldError(const std::string& key, bool error) const;

signals:
    void paramChanged(const std::string& key);

private:
    void setupUi();
    QWidget* createParamWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createFileWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createEnumWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createIntWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createNumberWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createBoolWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createTextWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createExtentWidget(const ParamSpec& spec, ParamWidgetEntry& entry);
    QWidget* createCrsWidget(const ParamSpec& spec, ParamWidgetEntry& entry);

    CardType cardType_;
    QFrame* cardFrame_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* iconLabel_ = nullptr;
    QGridLayout* paramsLayout_ = nullptr;
    QVBoxLayout* cardContentLayout_ = nullptr;
    int paramsRow_ = 0;

    QMap<std::string, ParamWidgetEntry> entries_;
    QMap<std::string, bool> requiredMap_;
};

}
