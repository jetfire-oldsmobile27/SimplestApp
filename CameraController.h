
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QCamera>
#include <QMediaDevices>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <memory>
#include <atomic>

class CameraController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString frameData READ frameData NOTIFY frameDataChanged)
    Q_PROPERTY(QStringList cameraIds READ cameraIds NOTIFY cameraIdsChanged)
    Q_PROPERTY(QStringList logs READ logs NOTIFY logsChanged)

public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    QString frameData() const;
    QStringList cameraIds() const;
    QStringList logs() const;

    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void refreshCameraList();
    Q_INVOKABLE void openCamera(int id);
    Q_INVOKABLE void closeCamera();

    static void registerInstance(CameraController *inst) { s_instance = inst; }
    static void qtMessageRouter(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    Q_INVOKABLE void receiveLoggedMessage(const QString &msg);

signals:
    void frameDataChanged();
    void cameraIdsChanged();
    void errorOccured(const QString &msg);
    void logsChanged();
    void cameraOperationFinished(); // ← новый сигнал

private slots:
    void processVideoFrame(const QVideoFrame &frame);

private:
    void applyImageProcessing(const QImage &inputImage);
    QStringList m_cameraIds;
    QString m_frameData;
    QStringList m_logs;
    std::atomic<bool> m_running{false};
    QCamera *m_camera = nullptr;
    QVideoSink *m_sink = nullptr;
    QMediaCaptureSession *m_captureSession = nullptr;
    QMutex m_mutex;

    static CameraController *s_instance;
};