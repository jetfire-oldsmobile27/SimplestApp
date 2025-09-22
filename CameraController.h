#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMutex>        // <- добавлено
#include <memory>        // для std::unique_ptr
#include <atomic>
#include <opencv2/opencv.hpp>

class CameraController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString frameData READ frameData NOTIFY frameDataChanged)
    Q_PROPERTY(QStringList cameraIds READ cameraIds NOTIFY cameraIdsChanged)
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    QString frameData() const;
    QStringList cameraIds() const;

    Q_INVOKABLE void refreshCameraList();     // найти доступные id
    Q_INVOKABLE void openCamera(int id);     // открыть выбранную
    Q_INVOKABLE void closeCamera();          // закрыть

signals:
    void frameDataChanged();
    void cameraIdsChanged();
    void errorOccured(const QString &msg);

private:
    void captureLoop(int id);
    QStringList m_cameraIds;
    QString m_frameData; // data:image/jpeg;base64,...
    std::atomic<bool> m_running;
    std::unique_ptr<QThread> m_workerThread;
    cv::VideoCapture m_cap;
    QMutex m_mutex;
};
