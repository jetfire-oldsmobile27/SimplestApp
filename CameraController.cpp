
#include "CameraController.h"
#include <QBuffer>
#include <QImage>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QGuiApplication>
#include <QPermissions>
#include <QMetaObject>
#include <QDateTime>
#include <QMediaCaptureSession> 
#include <opencv2/opencv.hpp>
#include <vector>

#ifdef Q_OS_ANDROID
 #include <QtCore/private/qandroidextras_p.h>
#endif

CameraController* CameraController::s_instance = nullptr;

void CameraController::qtMessageRouter(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString level;
    switch (type) {
        case QtDebugMsg: level = "DEBUG"; break;
        case QtInfoMsg: level = "INFO"; break;
        case QtWarningMsg: level = "WARNING"; break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg: level = "FATAL"; break;
        default: level = "UNKNOWN"; break;
    }

    QString file = context.file ? QString::fromLatin1(context.file) : QString();
    QString formatted = QString("[%1] %2").arg(level, msg);
    if (!file.isEmpty()) {
        formatted += QString(" (%1:%2)").arg(file).arg(context.line);
    }

    if (s_instance) {
        QMetaObject::invokeMethod(s_instance, "receiveLoggedMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, formatted));
    } else {
        QByteArray local = formatted.toLocal8Bit();
        fprintf(stderr, "%s\n", local.constData());
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void CameraController::receiveLoggedMessage(const QString &msg)
{
    qDebug() << "LOG RECEIVED:" << msg; // ← будет видно в adb logcat

    QString trimmed = msg;
    if (trimmed.length() > 500) {
        trimmed = trimmed.left(497) + "...";
    }

    m_logs.append(trimmed);
    if (m_logs.size() > 200) m_logs.removeFirst();
    emit logsChanged();

    if (msg.startsWith("[WARNING]") || msg.startsWith("[CRITICAL]") || msg.startsWith("[FATAL]")) {
        qDebug() << "EMITTING ERROR:" << trimmed;
        emit errorOccured(trimmed);
    }
}

QStringList CameraController::logs() const { return m_logs; }
void CameraController::clearLogs() {
    m_logs.clear();
    emit logsChanged();
}

CameraController::CameraController(QObject *parent)
    : QObject(parent)
{
    qDebug() << "CameraController created";
}

CameraController::~CameraController()
{
    qDebug() << "CameraController destroyed";
    closeCamera();
}

QString CameraController::frameData() const { return m_frameData; }
QStringList CameraController::cameraIds() const { return m_cameraIds; }

void CameraController::refreshCameraList()
{
    qDebug() << "refreshCameraList() called";

#ifdef Q_OS_ANDROID
    QCameraPermission cameraPermission;
    auto status = qApp->checkPermission(cameraPermission);
    if (status == Qt::PermissionStatus::Undetermined) {
        qDebug() << "Camera permission undetermined — requesting";
        qApp->requestPermission(cameraPermission, [this](const QPermission &perm){
            if (perm.status() == Qt::PermissionStatus::Granted) {
                QMetaObject::invokeMethod(this, "refreshCameraList", Qt::QueuedConnection);
            } else {
                qWarning() << "Camera permission denied by user";
                emit errorOccured("Camera permission denied");
            }
        });
        return;
    }
    if (status == Qt::PermissionStatus::Denied) {
        qWarning() << "Camera permission denied";
        emit errorOccured("Camera permission denied");
        return;
    }
#endif


    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QStringList ids;
    for (int i = 0; i < cameras.size(); ++i) {
        ids << QString::number(i);
    }

    m_cameraIds = ids;
    qDebug() << "Found cameras:" << m_cameraIds;
    emit cameraIdsChanged();
    emit cameraOperationFinished();
}

void CameraController::openCamera(int id)
{
    if (m_running.load()) {
    qWarning() << "Camera already running, ignoring open request";
    return;
    }
    qDebug() << "openCamera requested for id" << id;

#ifdef Q_OS_ANDROID
    QCameraPermission cameraPermission;
    auto status = qApp->checkPermission(cameraPermission);
    if (status == Qt::PermissionStatus::Undetermined) {
        qDebug() << "Asking for camera permission";
        qApp->requestPermission(cameraPermission, [this, id](const QPermission &perm){
            if (perm.status() == Qt::PermissionStatus::Granted) {
                QMetaObject::invokeMethod(this, "openCamera", Qt::QueuedConnection, Q_ARG(int, id));
            } else {
                qWarning() << "Camera permission denied";
                emit errorOccured("Camera permission denied");
            }
        });
        return;
    }
    if (status == Qt::PermissionStatus::Denied) {
        emit errorOccured("Camera permission denied");
        return;
    }
#endif

    closeCamera();

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (id < 0 || id >= cameras.size()) {
        emit errorOccured(QString("Invalid camera ID: %1").arg(id));
        return;
    }

    m_camera = new QCamera(cameras[id], this);
    m_sink = new QVideoSink(this);
    m_captureSession = new QMediaCaptureSession(this);

    m_captureSession->setCamera(m_camera);
    m_captureSession->setVideoSink(m_sink);

    connect(m_sink, &QVideoSink::videoFrameChanged, this, &CameraController::processVideoFrame);

    m_running.store(true);
    m_camera->start();

    if (m_camera->isActive()) {
        qDebug() << "Camera started successfully";
    } else {
        qWarning() << "Failed to start camera";
        emit errorOccured("Failed to start camera");
        closeCamera();
    }
    emit cameraOperationFinished();
}

void CameraController::closeCamera()
{
    if (!m_running.exchange(false)) {
        return; 
    }

    if (m_camera) {
        m_camera->stop();
        disconnect(m_sink, &QVideoSink::videoFrameChanged, this, &CameraController::processVideoFrame);
        m_camera->deleteLater();
        m_camera = nullptr;
    }
    if (m_sink) {
        m_sink->deleteLater();
        m_sink = nullptr;
    }
    if (m_captureSession) {
        m_captureSession->deleteLater();
        m_captureSession = nullptr;
    }
    qDebug() << "Camera closed";
    emit cameraOperationFinished();
}

void CameraController::processVideoFrame(const QVideoFrame &frame)
{
    if (!m_running.load() || !frame.isValid()) return;

    QImage image = frame.toImage();
    if (image.isNull()) return;

    applyImageProcessing(image);
}

void CameraController::applyImageProcessing(const QImage &inputImage)
{

    QImage rgbImage = inputImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3, rgbImage.bits(), rgbImage.bytesPerLine());


    std::vector<cv::Mat> ch;
    cv::split(mat, ch);
    ch[2] = ch[2] * 1.8f;  // Red
    ch[1] = ch[1] * 0.35f; // Green
    ch[0] = ch[0] * 0.35f; // Blue
    cv::merge(ch, mat);


    std::vector<uchar> buf;
    cv::imencode(".jpg", mat, buf, {cv::IMWRITE_JPEG_QUALITY, 70});

    QByteArray byteArray(reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()));
    QString base64 = QString::fromLatin1(byteArray.toBase64());

    QString dataUrl = "data:image/jpeg;base64," + base64 + "?ts=" + QString::number(QDateTime::currentMSecsSinceEpoch());

    {
        QMutexLocker locker(&m_mutex);
        if (m_frameData != dataUrl) {
            m_frameData = dataUrl;
            emit frameDataChanged();
        }
    }
}