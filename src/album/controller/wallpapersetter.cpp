#include "wallpapersetter.h"
#include "application.h"
#include "unionimage.h"
#include "baseutils.h"
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QImage>
#include <QDebug>
#include <QDBusInterface>
#include <QtDBus>
#include <QGuiApplication>
#include <QScreen>

#include <unistd.h>

namespace {
const QString WALLPAPER_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album" + QDir::separator() + "wallpapers" + QDir::separator();
}

WallpaperSetter *WallpaperSetter::m_setter = nullptr;
WallpaperSetter *WallpaperSetter::instance()
{
    if (! m_setter) {
        m_setter = new WallpaperSetter();
    }

    return m_setter;
}

WallpaperSetter::WallpaperSetter(QObject *parent) : QObject(parent)
{

}

bool WallpaperSetter::setBackground(const QString &pictureFilePath)
{
    QImage tImg;
    QString errMsg;
    QFileInfo info(pictureFilePath);
    QString tempWallPaperpath;
    tempWallPaperpath = WALLPAPER_PATH + info.fileName();
    QFileInfo tempInfo(tempWallPaperpath);
    if (!UnionImage_NameSpace::loadStaticImageFromFile(pictureFilePath, tImg, errMsg)) {
        qDebug() << "load wallpaper path error!";
        return false;
    }
    //临时文件目录不存在，先创建临时文件目录
    QDir tempImgDir(WALLPAPER_PATH);
    if (!tempImgDir.exists() && !tempImgDir.mkdir(tempImgDir.path())) {
//        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    if (!tImg.save(tempWallPaperpath, "JPG", 100)) {
//        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    if (!tempInfo.exists()) {
//        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    QDBusMessage msgIntrospect = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msgIntrospect);
    call.waitForFinished();
    if (call.isFinished()) {
        QDBusReply<QString> reply = call.reply();
        QString value = reply.value();
        if (value.contains("SetMonitorBackground")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "SetMonitorBackground");
            QString mm;
            if (Application::isWaylandPlatform()) {
                QDBusInterface interface("com.deepin.daemon.Display", "/com/deepin/daemon/Display", "com.deepin.daemon.Display");
                mm = qvariant_cast< QString >(interface.property("Primary"));
            } else {
                mm = qApp->primaryScreen()->name();
            }
            msg.setArguments({mm, tempWallPaperpath});
            QDBusConnection::sessionBus().asyncCall(msg);
            qDebug() << "FileUtils::setBackground call Appearance SetMonitorBackground";
            return true;
        }
    }
    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "Set");
    msg.setArguments({"Background", tempWallPaperpath});
    QDBusConnection::sessionBus().asyncCall(msg);
    return true;
}
