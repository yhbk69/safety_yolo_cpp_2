#ifndef SETTINGS_DIALOG_HPP
#define SETTINGS_DIALOG_HPP

#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;

struct SettingsResult {
    float confThreshold = 0.5f;
    float nmsThreshold = 0.45f;
    int websocketPort = 8765;
    int httpPort = 8080;
    int streamPort = 9090;
    int ackTimeoutMs = 5000;
    int alertCooldownMs = 3000;
    int ringBufferFrames = 150;
    QString modelPath;
    QString outputDir;
    QString recordDir;
};

class SettingsDialog : public QDialog {
public:
    explicit SettingsDialog(const SettingsResult& current, QWidget* parent = nullptr);

    SettingsResult result() const;

private:
    QDoubleSpinBox* confSpin_;
    QDoubleSpinBox* nmsSpin_;
    QSpinBox* wsPortSpin_;
    QSpinBox* httpPortSpin_;
    QSpinBox* streamPortSpin_;
    QSpinBox* ackSpin_;
    QSpinBox* cooldownSpin_;
    QSpinBox* ringSpin_;
    QLineEdit* modelEdit_;
    QLineEdit* outputEdit_;
    QLineEdit* recordEdit_;
};

#endif // SETTINGS_DIALOG_HPP
