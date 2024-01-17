/*
    SPDX-FileCopyrightText:  2015 LXQt team, 2024 Qtilities team
    SPDX-License-Identifier: LGPL-2.1-or-later

    This file is part of the statusnotifieritem-qt library
    Authors:
        Paulo Lieuthier <paulolieuthier@gmail.com>
*/
#include "statusnotifieritem.h"
#include "statusnotifieritemadaptor.h"
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <utility>
#include <dbusmenuexporter.h>

int StatusNotifierItem::mServiceCounter = 0;

StatusNotifierItem::StatusNotifierItem(QString id, QObject *parent)
    : QObject(parent),
    mAdaptor(new StatusNotifierItemAdaptor(this)),
    mService(QString::fromLatin1("org.freedesktop.StatusNotifierItem-%1-%2")
             .arg(QCoreApplication::applicationPid())
             .arg(++mServiceCounter)),
    mId(std::move(id)),
    mTitle(QLatin1String("Test")),
    mStatus(QLatin1String("Active")),
    mCategory(QLatin1String("ApplicationStatus")),
    mMenu(nullptr),
    mMenuPath(QLatin1String("/NO_DBUSMENU")),
    mMenuExporter(nullptr),
    mSessionBus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, mService))
{
    // Register DBus meta types
    qDBusRegisterMetaType<IconPixmap>();
    qDBusRegisterMetaType<IconPixmapList>();
    qDBusRegisterMetaType<ToolTip>();

    // Separate DBus connection to the session bus is created, because QDbus does not provide
    // a way to register different objects for different services with the same paths.
    // For status notifiers we need different /StatusNotifierItem for each service.

    // register service

    mSessionBus.registerObject(QLatin1String("/StatusNotifierItem"), this);

    registerToHost();

    // monitor the watcher service in case the host restarts
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(QLatin1String("org.kde.StatusNotifierWatcher"),
                                                           mSessionBus,
                                                           QDBusServiceWatcher::WatchForOwnerChange,
                                                           this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &StatusNotifierItem::onServiceOwnerChanged);
}

StatusNotifierItem::~StatusNotifierItem()
{
    mSessionBus.unregisterObject(QLatin1String("/StatusNotifierItem"));
    QDBusConnection::disconnectFromBus(mService);
}

QString StatusNotifierItem::id() const
{
    return mId;
}

QString StatusNotifierItem::title() const
{
    return mTitle;
}

void StatusNotifierItem::registerToHost()
{
    QDBusInterface interface(QLatin1String("org.kde.StatusNotifierWatcher"),
                             QLatin1String("/StatusNotifierWatcher"),
                             QLatin1String("org.kde.StatusNotifierWatcher"),
                             mSessionBus);
    interface.asyncCall(QLatin1String("RegisterStatusNotifierItem"), mSessionBus.baseService());
}

void StatusNotifierItem::onServiceOwnerChanged(const QString& service, const QString& oldOwner,
                                               const QString& newOwner)
{
	Q_UNUSED(service);
	Q_UNUSED(oldOwner);

    if (!newOwner.isEmpty())
        registerToHost();
}

void StatusNotifierItem::onMenuDestroyed()
{
    mMenu = nullptr;
    setMenuPath(QLatin1String("/NO_DBUSMENU"));
    mMenuExporter = nullptr; //mMenu is a QObject parent of the mMenuExporter
}

void StatusNotifierItem::setTitle(const QString &title)
{
    if (mTitle == title)
        return;

    mTitle = title;
    Q_EMIT mAdaptor->NewTitle();
}

QString StatusNotifierItem::status() const
{
    return mStatus;
}
void StatusNotifierItem::setStatus(const QString &status)
{
    if (mStatus == status)
        return;

    mStatus = status;
    Q_EMIT mAdaptor->NewStatus(mStatus);
}

QString StatusNotifierItem::category() const
{
    return mCategory;
}

void StatusNotifierItem::setCategory(const QString &category)
{
    if (mCategory == category)
        return;

    mCategory = category;
}

void StatusNotifierItem::setMenuPath(const QString& path)
{
    mMenuPath.setPath(path);
}

QDBusObjectPath StatusNotifierItem::menu() const
{
    return mMenuPath;
}

void StatusNotifierItem::setIconByName(const QString &name)
{
    if (mIconName == name)
        return;

    mIconName = name;
    Q_EMIT mAdaptor->NewIcon();
}

QString StatusNotifierItem::iconName() const
{
    return mIconName;
}

IconPixmapList StatusNotifierItem::iconPixmap() const
{
    return mIcon;
}

void StatusNotifierItem::setIconByPixmap(const QIcon &icon)
{
    if (mIconCacheKey == icon.cacheKey())
        return;

    mIconCacheKey = icon.cacheKey();
    mIcon = iconToPixmapList(icon);
    mIconName.clear();
    Q_EMIT mAdaptor->NewIcon();
}

QString StatusNotifierItem::overlayIconName() const
{
    return mOverlayIconName;
}

void StatusNotifierItem::setOverlayIconByName(const QString &name)
{
    if (mOverlayIconName == name)
        return;

    mOverlayIconName = name;
    Q_EMIT mAdaptor->NewOverlayIcon();
}

IconPixmapList StatusNotifierItem::overlayIconPixmap() const
{
    return mOverlayIcon;
}

void StatusNotifierItem::setOverlayIconByPixmap(const QIcon &icon)
{
    if (mOverlayIconCacheKey == icon.cacheKey())
        return;

    mOverlayIconCacheKey = icon.cacheKey();
    mOverlayIcon = iconToPixmapList(icon);
    mOverlayIconName.clear();
    Q_EMIT mAdaptor->NewOverlayIcon();
}

QString StatusNotifierItem::attentionIconName() const
{
    return mAttentionIconName;
}

void StatusNotifierItem::setAttentionIconByName(const QString &name)
{
    if (mAttentionIconName == name)
        return;

    mAttentionIconName = name;
    Q_EMIT mAdaptor->NewAttentionIcon();
}

IconPixmapList StatusNotifierItem::attentionIconPixmap() const
{
    return mAttentionIcon;
}

void StatusNotifierItem::setAttentionIconByPixmap(const QIcon &icon)
{
    if (mAttentionIconCacheKey == icon.cacheKey())
        return;

    mAttentionIconCacheKey = icon.cacheKey();
    mAttentionIcon = iconToPixmapList(icon);
    mAttentionIconName.clear();
    Q_EMIT mAdaptor->NewAttentionIcon();
}

QString StatusNotifierItem::toolTipTitle() const
{
    return mTooltipTitle;
}

void StatusNotifierItem::setToolTipTitle(const QString &title)
{
    if (mTooltipTitle == title)
        return;

    mTooltipTitle = title;
    Q_EMIT mAdaptor->NewToolTip();
}

QString StatusNotifierItem::toolTipSubTitle() const
{
    return mTooltipSubtitle;
}

void StatusNotifierItem::setToolTipSubTitle(const QString &subTitle)
{
    if (mTooltipSubtitle == subTitle)
        return;

    mTooltipSubtitle = subTitle;
    Q_EMIT mAdaptor->NewToolTip();
}

QString StatusNotifierItem::toolTipIconName() const
{
    return mTooltipIconName;
}

void StatusNotifierItem::setToolTipIconByName(const QString &name)
{
    if (mTooltipIconName == name)
        return;

    mTooltipIconName = name;
    Q_EMIT mAdaptor->NewToolTip();
}

IconPixmapList StatusNotifierItem::toolTipIconPixmap() const
{
    return mTooltipIcon;
}

void StatusNotifierItem::setToolTipIconByPixmap(const QIcon &icon)
{
    if (mTooltipIconCacheKey == icon.cacheKey())
        return;

    mTooltipIconCacheKey = icon.cacheKey();
    mTooltipIcon = iconToPixmapList(icon);
    mTooltipIconName.clear();
    Q_EMIT mAdaptor->NewToolTip();
}

void StatusNotifierItem::setContextMenu(QMenu* menu)
{
    if (mMenu == menu)
        return;

    if (nullptr != mMenu)
    {
        disconnect(mMenu, &QObject::destroyed, this, &StatusNotifierItem::onMenuDestroyed);
    }
    mMenu = menu;

    if (nullptr != mMenu)
        setMenuPath(QLatin1String("/MenuBar"));
    else
        setMenuPath(QLatin1String("/NO_DBUSMENU"));

    // Note: we need to destroy menu exporter before creating new one
    // to free the DBus object path for new menu
    delete mMenuExporter;
    if (nullptr != mMenu)
    {
        connect(mMenu, &QObject::destroyed, this, &StatusNotifierItem::onMenuDestroyed);
        mMenuExporter = new DBusMenuExporter{this->menu().path(), mMenu, mSessionBus};
    }
}

QMenu* StatusNotifierItem::contextMenu() const
{
    return mMenu;
}

void StatusNotifierItem::Activate(int x, int y)
{
    if (mStatus == QLatin1String("NeedsAttention"))
        mStatus = QLatin1String("Active");

    Q_EMIT activateRequested(QPoint(x, y));
}

void StatusNotifierItem::SecondaryActivate(int x, int y)
{
    if (mStatus == QLatin1String("NeedsAttention"))
        mStatus = QLatin1String("Active");

    Q_EMIT secondaryActivateRequested(QPoint(x, y));
}

void StatusNotifierItem::ContextMenu(int x, int y)
{
    if (mMenu != nullptr)
    {
        if (mMenu->isVisible())
            mMenu->popup(QPoint(x, y));
        else
            mMenu->hide();
    }
}

void StatusNotifierItem::Scroll(int delta, const QString &orientation)
{
    Qt::Orientation orient = Qt::Vertical;
    if (orientation.toLower() == QLatin1String("horizontal"))
        orient = Qt::Horizontal;

    Q_EMIT scrollRequested(delta, orient);
}

void StatusNotifierItem::showMessage(const QString& title, const QString& msg,
                                     const QString& iconName, int secs)
{
    QDBusInterface interface(QLatin1String("org.freedesktop.Notifications"), QLatin1String("/org/freedesktop/Notifications"),
                             QLatin1String("org.freedesktop.Notifications"), mSessionBus);
    interface.call(QLatin1String("Notify"), mTitle, (uint) 0, iconName, title,
                   msg, QStringList(), QVariantMap(), secs);
}

IconPixmapList StatusNotifierItem::iconToPixmapList(const QIcon& icon)
{
    IconPixmapList pixmapList;

    // long live KDE!
    const QList<QSize> sizes = icon.availableSizes();
    for (const QSize &size : sizes)
    {
        QImage image = icon.pixmap(size).toImage();

        IconPixmap pix;
        pix.height = image.height();
        pix.width = image.width();

        if (image.format() != QImage::Format_ARGB32)
            image = image.convertToFormat(QImage::Format_ARGB32);

        pix.bytes = QByteArray((char *) image.bits(),
                               image.sizeInBytes());

        // swap to network byte order if we are little endian
        if (QSysInfo::ByteOrder == QSysInfo::LittleEndian)
        {
            quint32 *uintBuf = (quint32 *) pix.bytes.data();
            for (uint i = 0; i < pix.bytes.size() / sizeof(quint32); ++i)
            {
                *uintBuf = qToBigEndian(*uintBuf);
                ++uintBuf;
            }
        }

        pixmapList.append(pix);
    }

    return pixmapList;
}
