#ifndef DETECTION_UTILS_HPP
#define DETECTION_UTILS_HPP

#include <string>
#include <QString>
#include <QColor>

inline QColor detectionColor(const std::string& className) {
    if (className.find("no_") == 0) return QColor("#d9534f");
    if (className == "Person" || className == "none") return QColor("#888888");
    return QColor("#5cb85c");
}

inline QString detectionText(const std::string& className, float conf) {
    return QString("%1  %2")
        .arg(QString::fromStdString(className), -12)
        .arg(conf, 0, 'f', 3);
}

#endif // DETECTION_UTILS_HPP
