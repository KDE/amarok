/*
 *   Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Petri Damsten <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "wallpaper.h"

#include <KServiceTypeTrader>
#include <KDebug>

#include <version.h>

namespace Plasma
{

class WallpaperPrivate
{
public:
    WallpaperPrivate(KService::Ptr service, Wallpaper *wallpaper) :
        q(wallpaper),
        wallpaperDescription(service),
        initialized(false)
    {
    };

    ~WallpaperPrivate()
    {
    };

    Wallpaper *q;
    KPluginInfo wallpaperDescription;
    QRectF boundingRect;
    KServiceAction mode;
    bool initialized;
};

Wallpaper::Wallpaper(QObject *parentObject, const QVariantList &args)
    : d(new WallpaperPrivate(KService::serviceByStorageId(args.count() > 0 ?
                             args[0].toString() : QString()), this))
{
    // now remove first item since those are managed by Wallpaper and subclasses shouldn't
    // need to worry about them. yes, it violates the constness of this var, but it lets us add
    // or remove items later while applets can just pretend that their args always start at 0
    QVariantList &mutableArgs = const_cast<QVariantList &>(args);
    if (!mutableArgs.isEmpty()) {
        mutableArgs.removeFirst();
    }
    setParent(parentObject);
}

Wallpaper::~Wallpaper()
{
    delete d;
}

KPluginInfo::List Wallpaper::listWallpaperInfo(const QString &formFactor)
{
    QString constraint;

    if (!formFactor.isEmpty()) {
        constraint.append("[X-Plasma-FormFactors] ~~ '").append(formFactor).append("'");
    }

    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Wallpaper", constraint);
    return KPluginInfo::fromServices(offers);
}

Wallpaper *Wallpaper::load(const QString &wallpaperName, const QVariantList &args)
{
    if (wallpaperName.isEmpty()) {
        return 0;
    }

    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(wallpaperName);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Wallpaper", constraint);

    if (offers.isEmpty()) {
        kDebug() << "offers is empty for " << wallpaperName;
        return 0;
    }

    KService::Ptr offer = offers.first();
    KPluginLoader plugin(*offer);

    if (!Plasma::isPluginVersionCompatible(plugin.pluginVersion())) {
        return 0;
    }

    QVariantList allArgs;
    allArgs << offer->storageId() << args;
    QString error;
    Wallpaper *wallpaper = offer->createInstance<Plasma::Wallpaper>(0, allArgs, &error);

    if (!wallpaper) {
        kDebug() << "Couldn't load wallpaper \"" << wallpaperName << "\"! reason given: " << error;
    }
    return wallpaper;
}

Wallpaper *Wallpaper::load(const KPluginInfo &info, const QVariantList &args)
{
    if (!info.isValid()) {
        return 0;
    }
    return load(info.pluginName(), args);
}

QString Wallpaper::name() const
{
    if (!d->wallpaperDescription.isValid()) {
        return i18n("Unknown Wallpaper");
    }

    return d->wallpaperDescription.name();
}

QString Wallpaper::icon() const
{
    if (!d->wallpaperDescription.isValid()) {
        return QString();
    }

    return d->wallpaperDescription.icon();
}

QString Wallpaper::pluginName() const
{
    if (!d->wallpaperDescription.isValid()) {
        return QString();
    }

    return d->wallpaperDescription.pluginName();
}

KServiceAction Wallpaper::renderingMode() const
{
    return d->mode;
}

QList<KServiceAction> Wallpaper::listRenderingModes() const
{
    if (!d->wallpaperDescription.isValid()) {
        return QList<KServiceAction>();
    }

    return d->wallpaperDescription.service()->actions();
}

QRectF Wallpaper::boundingRect() const
{
    return d->boundingRect;
}

bool Wallpaper::isInitialized() const
{
    return d->initialized;
}

void Wallpaper::setBoundingRect(const QRectF &boundingRect)
{
    d->boundingRect = boundingRect;
}

void Wallpaper::setRenderingMode(const QString &mode)
{
    if (d->mode.name() == mode) {
        return;
    }

    d->mode = KServiceAction();
    if (!mode.isEmpty()) {
        QList<KServiceAction> modes = listRenderingModes();

        foreach (const KServiceAction &action, modes) {
            if (action.name() == mode) {
                d->mode = action;
                break;
            }
        }
    }
}

void Wallpaper::restore(const KConfigGroup &config)
{
    d->initialized = true;
    init(config);
}

void Wallpaper::init(const KConfigGroup &config)
{
    Q_UNUSED(config);
}

void Wallpaper::save(KConfigGroup &config)
{
    Q_UNUSED(config);
}

QWidget *Wallpaper::createConfigurationInterface(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

void Wallpaper::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
}

void Wallpaper::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
}

void Wallpaper::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
}

void Wallpaper::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    Q_UNUSED(event)
}

} // Plasma namespace

#include "wallpaper.moc"
