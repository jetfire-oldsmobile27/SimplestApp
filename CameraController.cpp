#include "CameraController.h"
#include <QBuffer>
#include <QImage>
#include <QByteArray>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QMutexLocker>
#include <QGuiApplication>
#include <QPermissions> 

#ifdef Q_OS_ANDROID
 #include <QtCore/private/qandroidextras_p.h>
 #include <QPermissions>
#endif

CameraController::CameraController(QObject *parent)
    : QObject(parent), m_running(false)
{
    qDebug() << "CameraController created";
    // НЕ вызываем refreshCameraList() автоматически — ждём явного запроса от UI
    // refreshCameraList();
}

CameraController::~CameraController()
{
    qDebug() << "CameraController destroyed";
    closeCamera();
}

QString CameraController::frameData() const { return m_frameData; }
QStringList CameraController::cameraIds() const { return m_cameraIds; }

static bool permissionGrantedForCamera()
{
    QCameraPermission p;
    return qApp->checkPermission(p) == Qt::PermissionStatus::Granted;
}

void CameraController::refreshCameraList()
{
    qDebug() << "refreshCameraList() called";
    // На Android нельзя пробовать открывать камеру без runtime-привилегии.
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

    // Если дошли сюда — Granted. Теперь безопасно перечислить камеры.
    QStringList ids;
#ifdef Q_OS_ANDROID
    // На Android лучше не пытаться открывать VideoCapture без явного разрешения;
    // можно просто добавить индексы 0..N как опцию (VideoCapture при open всё равно проверит permission)
    // Здесь мы пробуем 0..5, но НЕ открываем их, чтобы не провоцировать CameraService ошибки.
    for (int i = 0; i < 6; ++i) ids << QString::number(i);
#else
    // Для десктопа пробуем реально открыть
    for (int i = 0; i < 6; ++i) {
        cv::VideoCapture cap;
#if defined(OPENCV_VIDEOIO_PRIORITY_MSMF)
        cap.open(i, cv::CAP_ANY);
#else
        cap.open(i);
#endif
        if (cap.isOpened()) {
            ids << QString::number(i);
            cap.release();
        }
    }
#endif

    m_cameraIds = ids;
    qDebug() << "Found cameras:" << m_cameraIds;
    emit cameraIdsChanged();
}

void CameraController::openCamera(int id)
{
    qDebug() << "openCamera requested for id" << id;

    // Проверяем / запрашиваем разрешение
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

    // Если уже запущено, сначала закрываем
    if (m_running.load()) {
        closeCamera();
    }

    m_running.store(true);

    m_workerThread.reset(new QThread());
    QObject *starter = new QObject();
    starter->moveToThread(m_workerThread.get());
    connect(m_workerThread.get(), &QThread::started, [this, id]() {
        this->captureLoop(id);
    });
    connect(m_workerThread.get(), &QThread::finished, starter, &QObject::deleteLater);
    m_workerThread->start();
}

void CameraController::closeCamera()
{
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(2000);
    }
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    m_workerThread.reset();
    qDebug() << "Camera closed";
}

void CameraController::captureLoop(int id)
{
    qDebug() << "captureLoop started for id" << id;
    try {
        // Попробуем безопасно открыть соответствующий backend
        {
            QMutexLocker locker(&m_mutex);
            bool opened = false;
#ifdef __ANDROID__
            opened = m_cap.open(id, cv::CAP_ANDROID);
            if (!opened) opened = m_cap.open(id, cv::CAP_ANDROID);
#endif
            if (!opened) opened = m_cap.open(id, cv::CAP_ANY);
        }

        if (!m_cap.isOpened()) {
            qWarning() << "Cannot open camera" << id;
            emit errorOccured(QString("Cannot open camera %1").arg(id));
            m_running.store(false);
            return;
        }

        cv::Mat frame;
        std::vector<uchar> buf;
        while (m_running.load()) {
            {
                QMutexLocker locker(&m_mutex);
                m_cap >> frame;
            }
            if (frame.empty()) {
                QThread::msleep(10);
                continue;
            }

            // Красная трансформация (как раньше)
            cv::Mat bgr;
            frame.copyTo(bgr);
            std::vector<cv::Mat> ch;
            cv::split(bgr, ch);
            ch[2].convertTo(ch[2], CV_32F);
            ch[1].convertTo(ch[1], CV_32F);
            ch[0].convertTo(ch[0], CV_32F);
            ch[2] = ch[2] * 1.8f;
            ch[1] = ch[1] * 0.35f;
            ch[0] = ch[0] * 0.35f;
            ch[2].convertTo(ch[2], CV_8U);
            ch[1].convertTo(ch[1], CV_8U);
            ch[0].convertTo(ch[0], CV_8U);
            cv::merge(ch, bgr);

            cv::imencode(".jpg", bgr, buf, {cv::IMWRITE_JPEG_QUALITY, 70});

            QByteArray byteArray(reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()));
            QString base64 = QString::fromLatin1(byteArray.toBase64());
            QString dataUrl = "data:image/jpeg;base64," + base64;

            QMetaObject::invokeMethod(this, [this, dataUrl]() {
                if (m_frameData != dataUrl) {
                    m_frameData = dataUrl;
                    emit frameDataChanged();
                }
            }, Qt::QueuedConnection);

            QThread::msleep(33);
        }
    } catch (const std::exception &ex) {
        qCritical() << "Exception in captureLoop:" << ex.what();
        emit errorOccured(QString("captureLoop exception: %1").arg(ex.what()));
    } catch (...) {
        qCritical() << "Unknown exception in captureLoop";
        emit errorOccured("Unknown exception in captureLoop");
    }

    {
        QMutexLocker locker(&m_mutex);
        if (m_cap.isOpened()) m_cap.release();
    }
    qDebug() << "captureLoop finished for id" << id;
}