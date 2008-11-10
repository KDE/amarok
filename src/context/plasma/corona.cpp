/*
 *   Copyright 2007 Matt Broadstone <mbroadst@gmail.com>
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 Riccardo Iaconelli <riccardo@kde.org>
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

#include "corona.h"

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsGridLayout>
#include <QMimeData>
#include <QPainter>
#include <QTimer>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KMimeType>

#include "containment.h"
#include "view.h"
#include "private/applet_p.h"
#include "tooltipmanager.h"

using namespace Plasma;

namespace Plasma
{

// constant controlling how long between requesting a configuration sync
// and one happening should occur. currently 10 seconds
const int CONFIG_SYNC_TIMEOUT = 10000;

class CoronaPrivate
{
public:
    CoronaPrivate(Corona *corona)
        : q(corona),
          immutability(Mutable),
          mimetype("text/x-plasmoidservicename"),
          config(0),
          offscreenLayout(0)
    {
        if (KGlobal::hasMainComponent()) {
            configName = KGlobal::mainComponent().componentName() + "-appletsrc";
        } else {
            configName = "plasma-appletsrc";
        }
    }

    ~CoronaPrivate()
    {
        qDeleteAll(containments);
    }

    void init()
    {
        configSyncTimer.setSingleShot(true);
        QObject::connect(&configSyncTimer, SIGNAL(timeout()), q, SLOT(syncConfig()));
    }

    void saveLayout(KSharedConfigPtr cg) const
    {
        KConfigGroup containmentsGroup(cg, "Containments");
        foreach (const Containment *containment, containments) {
            QString cid = QString::number(containment->id());
            KConfigGroup containmentConfig(&containmentsGroup, cid);
            containment->save(containmentConfig);
        }
    }

    void updateContainmentImmutability()
    {
        foreach (Containment *c, containments) {
            // we need to tell each containment that immutability has been altered
            c->updateConstraints(ImmutableConstraint);
        }
    }

    void containmentDestroyed(QObject *obj)
    {
        // we do a static_cast here since it really isn't an Containment by this
        // point anymore since we are in the qobject dtor. we don't actually
        // try and do anything with it, we just need the value of the pointer
        // so this unsafe looking code is actually just fine.
        Containment* containment = static_cast<Plasma::Containment*>(obj);
        int index = containments.indexOf(containment);

        if (index > -1) {
            containments.removeAt(index);
            q->requestConfigSync();
        }
    }

    void syncConfig()
    {
        q->config()->sync();
        emit q->configSynced();
    }

    Containment *addContainment(const QString &name, const QVariantList &args,
                                uint id, bool delayedInit)
    {
        QString pluginName = name;
        Containment *containment = 0;
        Applet *applet = 0;

        //kDebug() << "Loading" << name << args << id;

        if (pluginName.isEmpty()) {
            // default to the desktop containment
            pluginName = "desktop";
        }

        if (pluginName != "null") {
            applet = Applet::load(pluginName, id, args);
            containment = dynamic_cast<Containment*>(applet);
        }

        if (!containment) {
            kDebug() << "loading of containment" << name << "failed.";

            // in case we got a non-Containment from Applet::loadApplet or
            // a null containment was requested
            delete applet;
            containment = new Containment(0, 0, id);

            if (pluginName == "null") {
                containment->setDrawWallpaper(false);
            }

            // we want to provide something and don't care about the failure to launch
            containment->setFailedToLaunch(false);
            containment->setFormFactor(Plasma::Planar);
        }

        static_cast<Applet*>(containment)->d->setIsContainment(true);
        q->addItem(containment);

        if (!delayedInit) {
            containment->init();
            containment->updateConstraints(Plasma::StartupCompletedConstraint);
            KConfigGroup cg = containment->config();
            containment->save(cg);
            q->requestConfigSync();
        }

        containments.append(containment);
        QObject::connect(containment, SIGNAL(destroyed(QObject*)),
                         q, SLOT(containmentDestroyed(QObject*)));
        QObject::connect(containment, SIGNAL(configNeedsSaving()),
                         q, SLOT(requestConfigSync()));
        QObject::connect(containment, SIGNAL(releaseVisualFocus()),
                         q, SIGNAL(releaseVisualFocus()));
        QObject::connect(containment, SIGNAL(screenChanged(int,int,Plasma::Containment*)),
                         q, SIGNAL(screenOwnerChanged(int,int,Plasma::Containment*)));

        if (!delayedInit) {
            emit q->containmentAdded(containment);
        }

        return containment;
    }

    Corona *q;
    ImmutabilityType immutability;
    QString mimetype;
    QString configName;
    KSharedConfigPtr config;
    QTimer configSyncTimer;
    QList<Containment*> containments;
    QGraphicsGridLayout *offscreenLayout;
};

Corona::Corona(QObject *parent)
    : QGraphicsScene(parent),
      d(new CoronaPrivate(this))
{
    d->init();
    ToolTipManager::self()->m_corona = this;
    //setViewport(new QGLWidget(QGLFormat(QGL::StencilBuffer | QGL::AlphaChannel)));
}

Corona::~Corona()
{
    // FIXME: Same fix as in Plasma::View - make sure that when the focused widget is
    //        destroyed we don't try to transfer it to something that's already been
    //        deleted.
    clearFocus();

    KConfigGroup cg(config(), "General");

    // we call the dptr member directly for locked since isImmutable()
    // also checks kiosk and parent containers
    cg.writeEntry("immutability", (int)d->immutability);
    delete d;
}

void Corona::setAppletMimeType(const QString &type)
{
    d->mimetype = type;
}

QString Corona::appletMimeType()
{
    return d->mimetype;
}

void Corona::saveLayout(const QString &configName) const
{
    KSharedConfigPtr c;

    if (configName.isEmpty() || configName == d->configName) {
        c = config();
    } else {
        c = KSharedConfig::openConfig(configName);
    }

    d->saveLayout(c);
}

void Corona::requestConfigSync()
{
    // TODO: should we check into our immutability before doing this?

    //NOTE: this is a pretty simplistic model: we simply save no more than CONFIG_SYNC_TIMEOUT
    //      after the first time this is called. not much of a heuristic for save points, but
    //      it should at least compress these activities a bit and provide a way for applet
    //      authors to ween themselves from the sync() disease. A more interesting/dynamic
    //      algorithm for determining when to actually sync() to disk might be better, though.
    if (!d->configSyncTimer.isActive()) {
        d->configSyncTimer.start(CONFIG_SYNC_TIMEOUT);
    }
}

void Corona::initializeLayout(const QString &configName)
{
    clearContainments();
    loadLayout(configName);

    if (d->containments.isEmpty()) {
        loadDefaultLayout();
        if (!d->containments.isEmpty()) {
            requestConfigSync();
        }
    }

    if (config()->isImmutable()) {
        d->updateContainmentImmutability();
    }

    KConfigGroup coronaConfig(config(), "General");
    setImmutability((ImmutabilityType)coronaConfig.readEntry("immutability", (int)Mutable));
}

void Corona::loadLayout(const QString &configName)
{
    KSharedConfigPtr c;

    if (configName.isEmpty() || configName == d->configName) {
        c = config();
    } else {
        c = KSharedConfig::openConfig(configName);
    }

    KConfigGroup containments(config(), "Containments");

    foreach (const QString &group, containments.groupList()) {
        KConfigGroup containmentConfig(&containments, group);

        if (containmentConfig.entryMap().isEmpty()) {
            continue;
        }

        int cid = group.toUInt();
        //kDebug() << "got a containment in the config, trying to make a" << containmentConfig.readEntry("plugin", QString()) << "from" << group;
        Containment *c = d->addContainment(containmentConfig.readEntry("plugin", QString()), QVariantList(),
                                           cid, true);
        if (!c) {
            continue;
        }

        //addItem(c);
        c->init();
        c->restore(containmentConfig);
    }

    foreach (Containment *containment, d->containments) {
        QString cid = QString::number(containment->id());
        KConfigGroup containmentConfig(&containments, cid);

        foreach (Applet *applet, containment->applets()) {
            applet->init();
            // We have to flush the applet constraints manually
            applet->flushPendingConstraintsEvents();
        }

        containment->updateConstraints(Plasma::StartupCompletedConstraint);
        containment->flushPendingConstraintsEvents();
        emit containmentAdded(containment);
    }
}

Containment *Corona::containmentForScreen(int screen) const
{
    foreach (Containment *containment, d->containments) {
        if (containment->screen() == screen &&
            (containment->containmentType() == Containment::DesktopContainment ||
             containment->containmentType() >= Containment::CustomContainment)) {
            return containment;
        }
    }

    return 0;
}

QList<Containment*> Corona::containments() const
{
    return d->containments;
}

void Corona::clearContainments()
{
    foreach (Containment *containment, d->containments) {
        containment->clearApplets();
    }
}

KSharedConfigPtr Corona::config() const
{
    if (!d->config) {
        d->config = KSharedConfig::openConfig(d->configName);
    }

    return d->config;
}

Containment *Corona::addContainment(const QString &name, const QVariantList &args)
{
    return d->addContainment(name, args, 0, false);
}

Containment *Corona::addContainmentDelayed(const QString &name, const QVariantList &args)
{
    return d->addContainment(name, args, 0, true);
}

void Corona::addOffscreenWidget(QGraphicsWidget *widget)
{
    widget->setParentItem(0);
    if (!d->offscreenLayout) {
        kDebug() << "adding offscreen widget.";
        QGraphicsWidget *offscreenWidget = new QGraphicsWidget(0);
        addItem(offscreenWidget);
        d->offscreenLayout = new QGraphicsGridLayout(offscreenWidget);
        //FIXME: do this a nice way.
        offscreenWidget->setPos(-10000, -10000);
        offscreenWidget->setLayout(d->offscreenLayout);
    }

    //check if the layout already contains this widget.
    //XXX: duplicated from removeOffscreenWidget()
    for (int i = 0; i < d->offscreenLayout->count(); i++) {
        QGraphicsWidget *foundWidget = dynamic_cast<QGraphicsWidget*>(d->offscreenLayout->itemAt(i));
        if (foundWidget == widget) {
            return;
        }
    }

    d->offscreenLayout->addItem(widget, d->offscreenLayout->rowCount() + 1,
                                        d->offscreenLayout->columnCount() + 1);
    widget->update();
}

void Corona::removeOffscreenWidget(QGraphicsWidget *widget)
{
    if (!d->offscreenLayout) {
        return;
    }

    for (int i = 0; i < d->offscreenLayout->count(); i++) {
        QGraphicsWidget *foundWidget = dynamic_cast<QGraphicsWidget*>(d->offscreenLayout->itemAt(i));
        if (foundWidget == widget) {
            d->offscreenLayout->removeAt(i);
        }
    }
}

int Corona::numScreens() const
{
    return 1;
}

QRect Corona::screenGeometry(int id) const
{
    Q_UNUSED(id);
    if (views().isEmpty()) {
        return sceneRect().toRect();
    } else {
        QGraphicsView *v = views()[0];
        QRect r = sceneRect().toRect();
        r.moveTo(v->mapToGlobal(v->pos()));
        return r;
    }
}

QRegion Corona::availableScreenRegion(int id) const
{
    return QRegion(screenGeometry(id));
}

QPoint Corona::popupPosition(const QGraphicsItem *item, const QSize &s)
{
    QGraphicsView *v = viewFor(item);

    if (!v) {
        return QPoint(0, 0);
    }

    QPoint pos = v->mapFromScene(item->scenePos());
    pos = v->mapToGlobal(pos);
    //kDebug() << "==> position is" << item->scenePos() << v->mapFromScene(item->scenePos()) << pos;
    Plasma::View *pv = dynamic_cast<Plasma::View *>(v);

    Plasma::Location loc = Floating;
    if (pv && pv->containment()) {
        loc = pv->containment()->location();
    }

    switch (loc) {
    case BottomEdge:
        pos = QPoint(pos.x(), pos.y() - s.height());
        break;
    case TopEdge:
        pos = QPoint(pos.x(), pos.y() + (int)item->boundingRect().size().height());
        break;
    case LeftEdge:
        pos = QPoint(pos.x() + (int)item->boundingRect().size().width(), pos.y());
        break;
    case RightEdge:
        pos = QPoint(pos.x() - s.width(), pos.y());
        break;
    default:
        if (pos.y() - s.height() > 0) {
             pos = QPoint(pos.x(), pos.y() - s.height());
        } else {
             pos = QPoint(pos.x(), pos.y() + (int)item->boundingRect().size().height());
        }
    }

    //are we out of screen?
    QRect screenRect =
        screenGeometry((pv && pv->containment()) ? pv->containment()->screen() : -1);
    //kDebug() << "==> rect for" << (pv ? pv->containment()->screen() : -1) << "is" << screenRect;

    if (pos.rx() + s.width() > screenRect.right()) {
        pos.rx() -= ((pos.rx() + s.width()) - screenRect.right());
    }

    if (pos.ry() + s.height() > screenRect.bottom()) {
        pos.ry() -= ((pos.ry() + s.height()) - screenRect.bottom());
    }

    pos.rx() = qMax(0, pos.rx());
    return pos;
}



void Corona::loadDefaultLayout()
{
}

void Corona::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragEnterEvent(event);
}

void Corona::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragLeaveEvent(event);
}

void Corona::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragMoveEvent(event);
}

ImmutabilityType Corona::immutability() const
{
    return d->immutability;
}

void Corona::setImmutability(const ImmutabilityType immutable)
{
    if (d->immutability == immutable ||
        d->immutability == SystemImmutable) {
        return;
    }

    kDebug() << "setting immutability to" << immutable;
    d->immutability = immutable;
    d->updateContainmentImmutability();
}

} // namespace Plasma

#include "corona.moc"

