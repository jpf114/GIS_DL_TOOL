#include "icon_manager.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QFile>
#include <QTextStream>

namespace gis_ai::gui {

namespace {

std::string cacheKey(const std::string& svgName, IconWeight weight, int size, const QColor& color) {
    return svgName + "_" + (weight == IconWeight::Bold ? "bold" : "regular") + "_"
         + std::to_string(size) + "_" + color.name().toStdString();
}

}

IconManager& IconManager::instance() {
    static IconManager mgr;
    return mgr;
}

IconManager::IconManager() {
    loadMapping();
}

void IconManager::setIconsBasePath(const std::string& path) {
    basePath_ = path;
    cache_.clear();
}

void IconManager::loadMapping() {
    pluginMap_["segment"]    = {"scissors", IconWeight::Bold, QColor("#FFFFFF")};
    pluginMap_["inference"]  = {"brain",    IconWeight::Bold, QColor("#FFFFFF")};
    pluginMap_["preprocess"] = {"gear",     IconWeight::Bold, QColor("#FFFFFF")};
    pluginMap_["vector"]     = {"vector",   IconWeight::Bold, QColor("#FFFFFF")};
    pluginMap_["raster"]     = {"grid",     IconWeight::Bold, QColor("#FFFFFF")};
    pluginMap_["batch"]      = {"layers",   IconWeight::Bold, QColor("#FFFFFF")};

    actionMap_["tile_split"]     = {"layout",          IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["tile_merge"]     = {"merge",           IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["overlap_split"]  = {"copy",            IconWeight::Regular, QColor("#9AA8B8")};

    actionMap_["single_infer"]   = {"brain",           IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["batch_infer"]    = {"images",          IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["model_info"]     = {"info",            IconWeight::Regular, QColor("#9AA8B8")};

    actionMap_["normalize"]      = {"chart-line-up",   IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["resample"]       = {"arrows-clockwise",IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["clip"]           = {"scissors",        IconWeight::Regular, QColor("#9AA8B8")};

    actionMap_["mask_to_polygon"] = {"polygon",        IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["vector_clip"]     = {"scissors",       IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["vector_buffer"]   = {"circle",         IconWeight::Regular, QColor("#9AA8B8")};

    actionMap_["raster_clip"]      = {"scissors",      IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["raster_mosaic"]    = {"squares-four",  IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["raster_threshold"] = {"chart-line-up", IconWeight::Regular, QColor("#9AA8B8")};

    actionMap_["batch_segment"]  = {"layers",          IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["batch_inference"]= {"brain",           IconWeight::Regular, QColor("#9AA8B8")};
    actionMap_["task_queue"]     = {"stack",           IconWeight::Regular, QColor("#9AA8B8")};

    cardMap_["input"]    = {"arrow-down",  IconWeight::Regular, QColor("#2F7CF6")};
    cardMap_["output"]   = {"arrow-up",    IconWeight::Regular, QColor("#2F7CF6")};
    cardMap_["advanced"] = {"sliders",     IconWeight::Regular, QColor("#2F7CF6")};
}

QIcon IconManager::iconForPlugin(const std::string& pluginName, const QColor& color) {
    return QIcon(pixmapForPlugin(pluginName, 18, color));
}

QIcon IconManager::iconForAction(const std::string& actionKey, const QColor& color) {
    return QIcon(pixmapForAction(actionKey, 16, color));
}

QIcon IconManager::iconForCard(const std::string& cardType, const QColor& color) {
    return QIcon(pixmapForCard(cardType, 16, color));
}

QPixmap IconManager::pixmapForPlugin(const std::string& pluginName, int size, const QColor& color) {
    auto it = pluginMap_.find(pluginName);
    if (it == pluginMap_.end()) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(color, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(2, 2, size - 4, size - 4));
        return pixmap;
    }
    const auto& spec = it->second;
    const std::string key = cacheKey(spec.svgName, spec.weight, size, color);
    auto cacheIt = cache_.find(key);
    if (cacheIt != cache_.end()) {
        return cacheIt->second;
    }
    const std::string path = svgPathFor(spec.svgName, spec.weight);
    QPixmap rendered = renderSvg(path, size, color);
    cache_[key] = rendered;
    return rendered;
}

QPixmap IconManager::pixmapForAction(const std::string& actionKey, int size, const QColor& color) {
    auto it = actionMap_.find(actionKey);
    if (it == actionMap_.end()) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(color, 1.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(3, 3, size - 6, size - 6));
        return pixmap;
    }
    const auto& spec = it->second;
    const std::string key = cacheKey(spec.svgName, spec.weight, size, color);
    auto cacheIt = cache_.find(key);
    if (cacheIt != cache_.end()) {
        return cacheIt->second;
    }
    const std::string path = svgPathFor(spec.svgName, spec.weight);
    QPixmap rendered = renderSvg(path, size, color);
    cache_[key] = rendered;
    return rendered;
}

QPixmap IconManager::pixmapForAction(const std::string& pluginName, const std::string& actionKey,
                                     int size, const QColor& color) {
    const std::string compositeKey = pluginName + ":" + actionKey;
    auto it = actionMap_.find(compositeKey);
    if (it == actionMap_.end()) {
        it = actionMap_.find(actionKey);
    }
    if (it == actionMap_.end()) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(color, 1.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(3, 3, size - 6, size - 6));
        return pixmap;
    }
    const auto& spec = it->second;
    const std::string key = cacheKey(spec.svgName, spec.weight, size, color);
    auto cacheIt = cache_.find(key);
    if (cacheIt != cache_.end()) {
        return cacheIt->second;
    }
    const std::string path = svgPathFor(spec.svgName, spec.weight);
    QPixmap rendered = renderSvg(path, size, color);
    cache_[key] = rendered;
    return rendered;
}

QPixmap IconManager::pixmapForCard(const std::string& cardType, int size, const QColor& color) {
    auto it = cardMap_.find(cardType);
    if (it == cardMap_.end()) {
        return pixmapForAction("info", size, color);
    }
    const auto& spec = it->second;
    const std::string key = cacheKey(spec.svgName, spec.weight, size, color);
    auto cacheIt = cache_.find(key);
    if (cacheIt != cache_.end()) {
        return cacheIt->second;
    }
    const std::string path = svgPathFor(spec.svgName, spec.weight);
    QPixmap rendered = renderSvg(path, size, color);
    cache_[key] = rendered;
    return rendered;
}

bool IconManager::hasPluginIcon(const std::string& pluginName) const {
    return pluginMap_.find(pluginName) != pluginMap_.end();
}

bool IconManager::hasActionIcon(const std::string& actionKey) const {
    return actionMap_.find(actionKey) != actionMap_.end();
}

bool IconManager::hasActionIcon(const std::string& pluginName, const std::string& actionKey) const {
    const std::string compositeKey = pluginName + ":" + actionKey;
    return actionMap_.find(compositeKey) != actionMap_.end()
        || actionMap_.find(actionKey) != actionMap_.end();
}

std::string IconManager::svgPathFor(const std::string& svgName, IconWeight weight) const {
    const std::string weightDir = (weight == IconWeight::Bold) ? "bold" : "regular";
    const std::string suffix = (weight == IconWeight::Bold) ? "-bold" : "-regular";
    return basePath_ + "/" + weightDir + "/" + svgName + suffix + ".svg";
}

QPixmap IconManager::renderSvg(const std::string& svgPath, int size, const QColor& color) {
    QFile file(QString::fromStdString(svgPath));
    if (!file.open(QIODevice::ReadOnly)) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        return pixmap;
    }

    QString svgContent = QTextStream(&file).readAll();
    file.close();

    svgContent.replace("stroke=\"currentColor\"", QString("stroke=\"%1\"").arg(color.name()));
    svgContent.replace("fill=\"currentColor\"", QString("fill=\"%1\"").arg(color.name()));

    if (!svgContent.contains("color=\"")) {
        svgContent.replace("<svg", QString("<svg color=\"%1\" ").arg(color.name()));
    }

    QSvgRenderer renderer(svgContent.toUtf8());
    if (!renderer.isValid()) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        return pixmap;
    }

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, size, size));
    return pixmap;
}

}
