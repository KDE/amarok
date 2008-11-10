/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Ménard Alexis <darktears31@gmail.com>
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

#include "containment.h"
#include "private/containment_p.h"

#include <QAction>
#include <QFile>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsLayout>
#include <QGraphicsLinearLayout>

#include <KAction>
#include <KApplication>
#include <KAuthorized>
#include <KIcon>
#include <KMenu>
#include <KMessageBox>
#include <KMimeType>
#include <KRun>
#include <KServiceTypeTrader>
#include <KStandardDirs>
#include <KTemporaryFile>
#include <KWindowSystem>

#include "animator.h"
#include "context.h"
#include "corona.h"
#include "svg.h"
#include "wallpaper.h"

#include "private/applet_p.h"
#include "private/applethandle_p.h"
#include "private/desktoptoolbox_p.h"
#include "private/paneltoolbox_p.h"

namespace Plasma
{

bool ContainmentPrivate::s_positioning = false;
static const char defaultWallpaper[] = "image";
static const char defaultWallpaperMode[] = "SingleImage";

Containment::StyleOption::StyleOption()
    : QStyleOptionGraphicsItem(),
      view(0)
{
    version = Version;
    type = Type;
}

Containment::StyleOption::StyleOption(const Containment::StyleOption & other)
    : QStyleOptionGraphicsItem(other),
      view(other.view)
{
    version = Version;
    type = Type;
}

Containment::StyleOption::StyleOption(const QStyleOptionGraphicsItem &other)
    : QStyleOptionGraphicsItem(other),
      view(0)
{
    version = Version;
    type = Type;
}

Containment::Containment(QGraphicsItem *parent,
                         const QString &serviceId,
                         uint containmentId)
    : Applet(parent, serviceId, containmentId),
      d(new ContainmentPrivate(this))
{
    // WARNING: do not access config() OR globalConfig() in this method!
    //          that requires a scene, which is not available at this point
    setPos(0, 0);
    setBackgroundHints(NoBackground);
    setContainmentType(CustomContainment);
}

Containment::Containment(QObject *parent, const QVariantList &args)
    : Applet(parent, args),
      d(new ContainmentPrivate(this))
{
    // WARNING: do not access config() OR globalConfig() in this method!
    //          that requires a scene, which is not available at this point
    setPos(0, 0);
    setBackgroundHints(NoBackground);
}

Containment::~Containment()
{
    delete d;
}

void Containment::init()
{
    if (!isContainment()) {
        return;
    }

    setCacheMode(NoCache);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    setAcceptDrops(true);
    setAcceptsHoverEvents(true);

    //TODO: would be nice to not do this on init, as it causes Animator to init
    connect(Animator::self(), SIGNAL(animationFinished(QGraphicsItem*,Plasma::Animator::Animation)),
            this, SLOT(containmentAppletAnimationComplete(QGraphicsItem*,Plasma::Animator::Animation)));

    if (d->type == NoContainmentType) {
        setContainmentType(DesktopContainment);
    }

    //common actions
    bool unlocked = immutability() == Mutable;

    QAction *appletBrowserAction = new QAction(i18n("Add Widgets..."), this);
    appletBrowserAction->setIcon(KIcon("list-add"));
    appletBrowserAction->setVisible(unlocked);
    appletBrowserAction->setEnabled(unlocked);
    connect(appletBrowserAction, SIGNAL(triggered()), this, SLOT(triggerShowAddWidgets()));
    appletBrowserAction->setShortcutContext(Qt::WidgetShortcut);
    appletBrowserAction->setShortcut(QKeySequence("ctrl+a"));
    d->actions().addAction("add widgets", appletBrowserAction);

    QAction *action = new QAction(i18n("Next Widget"), this);
    //no icon
    connect(action, SIGNAL(triggered()), this, SLOT(focusNextApplet()));
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setShortcut(QKeySequence("ctrl+n"));
    d->actions().addAction("next applet", action);

    action = new QAction(i18n("Previous Widget"), this);
    //no icon
    connect(action, SIGNAL(triggered()), this, SLOT(focusPreviousApplet()));
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setShortcut(QKeySequence("ctrl+p"));
    d->actions().addAction("previous applet", action);

    if (immutability() != SystemImmutable) {
        //FIXME I'm not certain this belongs in Containment
        //but it sure is nice to have the keyboard shortcut in every containment by default
        QAction *lockDesktopAction =
            new QAction(unlocked ? i18n("Lock Widgets") : i18n("Unlock Widgets"), this);
        lockDesktopAction->setIcon(KIcon(unlocked ? "object-locked" : "object-unlocked"));
        connect(lockDesktopAction, SIGNAL(triggered(bool)),
                this, SLOT(toggleDesktopImmutability()));
        lockDesktopAction->setShortcutContext(Qt::WidgetShortcut);
        lockDesktopAction->setShortcut(QKeySequence("ctrl+l"));
        d->actions().addAction("lock widgets", lockDesktopAction);
    }

    if (d->type != PanelContainment &&
        d->type != CustomPanelContainment) {
        QAction *zoomAction = new QAction(i18n("Zoom In"), this);
        zoomAction->setIcon(KIcon("zoom-in"));
        connect(zoomAction, SIGNAL(triggered(bool)), this, SLOT(zoomIn()));
        zoomAction->setShortcutContext(Qt::WidgetShortcut);
        //two shortcuts because I hate ctrl-+ but others expect it
        QList<QKeySequence> keys;
        keys << QKeySequence(QKeySequence::ZoomIn);
        keys << QKeySequence("ctrl+=");
        zoomAction->setShortcuts(keys);
        d->actions().addAction("zoom in", zoomAction);

        zoomAction = new QAction(i18n("Zoom Out"), this);
        zoomAction->setIcon(KIcon("zoom-out"));
        connect(zoomAction, SIGNAL(triggered(bool)), this, SLOT(zoomOut()));
        zoomAction->setShortcutContext(Qt::WidgetShortcut);
        zoomAction->setShortcut(QKeySequence(QKeySequence::ZoomOut));
        d->actions().addAction("zoom out", zoomAction);

        QAction *activityAction = new QAction(i18n("Add Activity"), this);
        activityAction->setIcon(KIcon("list-add"));
        activityAction->setVisible(unlocked);
        activityAction->setEnabled(unlocked);
        connect(activityAction, SIGNAL(triggered(bool)), this, SLOT(addSiblingContainment()));
        activityAction->setShortcutContext(Qt::WidgetShortcut);
        activityAction->setShortcut(QKeySequence("ctrl+shift+a"));
        d->actions().addAction("add sibling containment", activityAction);

        if (d->type == DesktopContainment && d->toolBox) {
            d->toolBox->addTool(this->action("add widgets"));
            d->toolBox->addTool(this->action("zoom in"));
            d->toolBox->addTool(this->action("zoom out"));
            if (immutability() != SystemImmutable) {
                d->toolBox->addTool(this->action("lock widgets"));
            }
            d->toolBox->addTool(this->action("add sibling containment"));
            if (hasConfigurationInterface()) {
                // re-use the contianment's action.
                QAction *configureContainment = this->action("configure");
                if (configureContainment) {
                    d->toolBox->addTool(this->action("configure"));
                }
            }

        }

        //Set a default wallpaper the first time the containment is created,
        //for instance from the toolbox by the user
        if (d->drawWallpaper) {
            setDrawWallpaper(true);
        }
    }
}

// helper function for sorting the list of applets
bool appletConfigLessThan(const KConfigGroup &c1, const KConfigGroup &c2)
{
    QPointF p1 = c1.readEntry("geometry", QRectF()).topLeft();
    QPointF p2 = c2.readEntry("geometry", QRectF()).topLeft();
    if (p1.x() != p2.x()) {
        return p1.x() < p2.x();
    }
    return p1.y() < p2.y();
}

void Containment::restore(KConfigGroup &group)
{
    /*kDebug() << "!!!!!!!!!!!!initConstraints" << group.name() << containmentType();
    kDebug() << "    location:" << group.readEntry("location", (int)d->location);
    kDebug() << "    geom:" << group.readEntry("geometry", geometry());
    kDebug() << "    formfactor:" << group.readEntry("formfactor", (int)d->formFactor);
    kDebug() << "    screen:" << group.readEntry("screen", d->screen);*/
    if (!isContainment()) {
        Applet::restore(group);
        return;
    }

    QRectF geo = group.readEntry("geometry", geometry());
    //override max/min
    //this ensures panels are set to their saved size even when they have max & min set to prevent
    //resizing
    if (geo.size() != geo.size().boundedTo(maximumSize())) {
        setMaximumSize(maximumSize().expandedTo(geo.size()));
    }
    if (geo.size() != geo.size().expandedTo(minimumSize())) {
        setMinimumSize(minimumSize().boundedTo(geo.size()));
    }
    setGeometry(geo);

    setLocation((Plasma::Location)group.readEntry("location", (int)d->location));
    setFormFactor((Plasma::FormFactor)group.readEntry("formfactor", (int)d->formFactor));
    setScreen(group.readEntry("screen", d->screen));
    setActivity(group.readEntry("activity", QString()));

    flushPendingConstraintsEvents();
    restoreContents(group);
    setImmutability((ImmutabilityType)group.readEntry("immutability", (int)Mutable));

    setWallpaper(group.readEntry("wallpaperplugin", defaultWallpaper),
                 group.readEntry("wallpaperpluginmode", defaultWallpaperMode));
    /*
    kDebug() << "Containment" << id() <<
                "screen" << screen() <<
                "geometry is" << geometry() <<
                "wallpaper" << ((d->wallpaper) ? d->wallpaper->pluginName() : QString()) <<
                "wallpaper mode" << wallpaperMode() <<
                "config entries" << group.entryMap();
    */
}

void Containment::save(KConfigGroup &g) const
{
    KConfigGroup group = g;
    if (!group.isValid()) {
        group = config();
    }

    // locking is saved in Applet::save
    Applet::save(group);
    group.writeEntry("screen", d->screen);
    group.writeEntry("formfactor", (int)d->formFactor);
    group.writeEntry("location", (int)d->location);
    group.writeEntry("activity", d->context()->currentActivity());

    if (d->wallpaper) {
        group.writeEntry("wallpaperplugin", d->wallpaper->pluginName());
        group.writeEntry("wallpaperpluginmode", d->wallpaper->renderingMode().name());

        if (d->wallpaper->isInitialized()) {
            KConfigGroup wallpaperConfig(&group, "Wallpaper");
            wallpaperConfig = KConfigGroup(&wallpaperConfig, d->wallpaper->pluginName());
            d->wallpaper->save(wallpaperConfig);
        }
    }

    saveContents(group);
}

void Containment::saveContents(KConfigGroup &group) const
{
    KConfigGroup applets(&group, "Applets");
    foreach (const Applet *applet, d->applets) {
        KConfigGroup appletConfig(&applets, QString::number(applet->id()));
        applet->save(appletConfig);
    }
}

void Containment::restoreContents(KConfigGroup &group)
{
    KConfigGroup applets(&group, "Applets");

    // Sort the applet configs in order of geometry to ensure that applets
    // are added from left to right or top to bottom for a panel containment
    QList<KConfigGroup> appletConfigs;
    foreach (const QString &appletGroup, applets.groupList()) {
        //kDebug() << "reading from applet group" << appletGroup;
        KConfigGroup appletConfig(&applets, appletGroup);
        appletConfigs.append(appletConfig);
    }
    qSort(appletConfigs.begin(), appletConfigs.end(), appletConfigLessThan);

    foreach (KConfigGroup appletConfig, appletConfigs) {
        int appId = appletConfig.name().toUInt();
        QString plugin = appletConfig.readEntry("plugin", QString());

        if (plugin.isEmpty()) {
            continue;
        }

        Applet *applet =
            d->addApplet(plugin, QVariantList(),
                         appletConfig.readEntry("geometry", QRectF()), appId, true);
        applet->restore(appletConfig);
    }
}

Containment::Type Containment::containmentType() const
{
    return d->type;
}

void Containment::setContainmentType(Containment::Type type)
{
    if (d->type == type) {
        return;
    }

    delete d->toolBox;
    d->toolBox = 0;
    d->type = type;

    if (!isContainment()) {
        return;
    }

    if ((type == DesktopContainment || type == PanelContainment)) {
        d->createToolBox();
    }
}

Corona *Containment::corona() const
{
    return dynamic_cast<Corona*>(scene());
}

void Containment::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    if (d->wallpaper && d->wallpaper->isInitialized()) {
        QGraphicsItem *item = scene()->itemAt(event->scenePos());
        if (item == this) {
            d->wallpaper->mouseMoveEvent(event);
        }
    }

    if (!event->isAccepted()) {
        event->accept();
        Applet::mouseMoveEvent(event);
    }
}

void Containment::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    if (d->wallpaper && d->wallpaper->isInitialized()) {
        QGraphicsItem *item = scene()->itemAt(event->scenePos());
        if (item == this) {
            d->wallpaper->mousePressEvent(event);
        }
    }

    if (event->isAccepted()) {
        setFocus(Qt::MouseFocusReason);
    } else {
        event->accept();
        Applet::mousePressEvent(event);
    }
}

void Containment::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    if (d->wallpaper && d->wallpaper->isInitialized()) {
        QGraphicsItem *item = scene()->itemAt(event->scenePos());
        if (item == this) {
            d->wallpaper->mouseReleaseEvent(event);
        }
    }

    if (!event->isAccepted()) {
        event->accept();
        Applet::mouseReleaseEvent(event);
    }
}

void Containment::showDropZone(const QPoint pos)
{
    //Base implementation does nothing, don't put code here
}

void Containment::showContextMenu(const QPointF &containmentPos, const QPoint &screenPos)
{
    d->showContextMenu(mapToScene(containmentPos), screenPos, false);
}

void Containment::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    //kDebug() << "let's see if we manage to get a context menu here, huh";
    if (!isContainment() || !scene() || !KAuthorized::authorizeKAction("desktop_contextmenu")) {
        Applet::contextMenuEvent(event);
        return;
    }

    if (!d->showContextMenu(event->scenePos(), event->screenPos(), true)) {
        Applet::contextMenuEvent(event);
    } else {
        event->accept();
    }
}

bool ContainmentPrivate::showContextMenu(const QPointF &point,
                                         const QPoint &screenPos, bool includeApplet)
{
    Applet *applet = 0;

    QGraphicsItem *item = q->scene()->itemAt(point);
    if (item == q) {
        item = 0;
    }

    while (item) {
        applet = qgraphicsitem_cast<Applet*>(item);
        if (applet && !applet->isContainment()) {
            break;
        }

        // applet may have a value due to finding a containment!
        applet = 0;
        item = item->parentItem();
    }

    KMenu desktopMenu;
    //kDebug() << "context menu event " << (QObject*)applet;
    if (applet) {
        bool hasEntries = false;
        QList<QAction*> actions;

        if (includeApplet) {
            actions = applet->contextualActions();
            if (!actions.isEmpty()) {
                foreach (QAction *action, actions) {
                    if (action) {
                        desktopMenu.addAction(action);
                    }
                }
                hasEntries = true;
            }
        }

        if (applet->hasConfigurationInterface()) {
            QAction *configureApplet = applet->d->actions.action("configure");
            if (configureApplet) {
                desktopMenu.addAction(configureApplet);
                hasEntries = true;
            }
        }

        QList<QAction*> containmentActions = q->contextualActions();
        if (!containmentActions.isEmpty()) {
            if (hasEntries) {
                desktopMenu.addSeparator();
            }

            hasEntries = true;
            QMenu *containmentActionMenu = &desktopMenu;

            if (!actions.isEmpty() && containmentActions.count() > 2) {
                containmentActionMenu = new KMenu(i18nc("%1 is the name of the containment", "%1 Options", q->name()), &desktopMenu);
                desktopMenu.addMenu(containmentActionMenu);
            }

            foreach (QAction *action, containmentActions) {
                if (action) {
                    containmentActionMenu->addAction(action);
                }
            }
        }

        if (static_cast<Corona*>(q->scene())->immutability() == Mutable) {
            if (hasEntries) {
                desktopMenu.addSeparator();
            }

            QAction *closeApplet = applet->d->actions.action("remove");
            if (!closeApplet) { //unlikely but not impossible
                kDebug() << "no remove action!!!!!!!!";
                closeApplet = new QAction(i18nc("%1 is the name of the applet", "Remove this %1", applet->name()), &desktopMenu);
                closeApplet->setIcon(KIcon("edit-delete"));
                QObject::connect(closeApplet, SIGNAL(triggered(bool)), applet, SLOT(destroy()));
            }
            desktopMenu.addAction(closeApplet);
            hasEntries = true;
        }

        if (!hasEntries) {
            kDebug() << "no entries";
            return false;
        }
    } else {
        if (static_cast<Corona*>(q->scene())->immutability() != Mutable &&
            !KAuthorized::authorizeKAction("unlock_desktop")) {
            //kDebug() << "immutability";
            return false;
        }

        QList<QAction*> actions = q->contextualActions();

        if (actions.count() < 1) {
            //kDebug() << "no applet, but no actions";
            return false;
        }

        foreach (QAction *action, actions) {
            if (action) {
                desktopMenu.addAction(action);
            }
        }
    }

    //kDebug() << "executing at" << screenPos;
    desktopMenu.exec(screenPos);
    return true;
}

void Containment::setFormFactor(FormFactor formFactor)
{
    if (d->formFactor == formFactor) {
        return;
    }

    //kDebug() << "switching FF to " << formFactor;
    FormFactor was = d->formFactor;
    d->formFactor = formFactor;

    if (isContainment() &&
        was != formFactor &&
        (d->type == PanelContainment ||
         d->type == CustomPanelContainment)) {
        // we are a panel and we have chaged our orientation
        d->positionPanel(true);
    }

    updateConstraints(Plasma::FormFactorConstraint);

    KConfigGroup c = config();
    c.writeEntry("formfactor", (int)formFactor);
    emit configNeedsSaving();
}

void Containment::setLocation(Location location)
{
    if (d->location == location) {
        return;
    }

    bool emitGeomChange = false;

    if ((location == TopEdge || location == BottomEdge) &&
        (d->location == TopEdge || d->location == BottomEdge)) {
        emitGeomChange = true;
    }

    if ((location == RightEdge || location == LeftEdge) &&
        (d->location == RightEdge || d->location == LeftEdge)) {
        emitGeomChange = true;
    }

    d->location = location;

    foreach (Applet *applet, d->applets) {
        applet->updateConstraints(Plasma::LocationConstraint);
    }

    if (emitGeomChange) {
        // our geometry on the scene will not actually change,
        // but for the purposes of views it has
        emit geometryChanged();
    }

    updateConstraints(Plasma::LocationConstraint);

    KConfigGroup c = config();
    c.writeEntry("location", (int)location);
    emit configNeedsSaving();
}

void Containment::addSiblingContainment()
{
    emit addSiblingContainment(this);
}

void Containment::clearApplets()
{
    foreach (Applet *applet, d->applets) {
        applet->d->cleanUpAndDelete();
    }

    d->applets.clear();
}

Applet *Containment::addApplet(const QString &name, const QVariantList &args,
                               const QRectF &appletGeometry)
{
    return d->addApplet(name, args, appletGeometry);
}

void Containment::addApplet(Applet *applet, const QPointF &pos, bool delayInit)
{
    if (!isContainment() || (!delayInit && immutability() != Mutable)) {
        return;
    }

    if (!applet) {
        kDebug() << "adding null applet!?!";
        return;
    }

    if (d->applets.contains(applet)) {
        kDebug() << "already have this applet!";
    }

    Containment *currentContainment = applet->containment();

    if (containmentType() == PanelContainment) {
        //panels don't want backgrounds, which is important when setting geometry
        setBackgroundHints(NoBackground);
    }

    if (currentContainment && currentContainment != this) {
        emit currentContainment->appletRemoved(applet);
        applet->removeSceneEventFilter(currentContainment);
        KConfigGroup oldConfig = applet->config();
        currentContainment->d->applets.removeAll(applet);
        if (currentContainment->d->handles.contains(applet)) {
            currentContainment->d->handles.remove(applet);
        }
        applet->setParentItem(this);

        // now move the old config to the new location
        KConfigGroup c = config().group("Applets").group(QString::number(applet->id()));
        oldConfig.reparent(&c);
        applet->d->resetConfigurationObject();
    } else {
        applet->setParentItem(this);
    }

    d->applets << applet;

    connect(applet, SIGNAL(configNeedsSaving()), this, SIGNAL(configNeedsSaving()));
    connect(applet, SIGNAL(releaseVisualFocus()), this, SIGNAL(releaseVisualFocus()));
    connect(applet, SIGNAL(destroyed(QObject*)), this, SLOT(appletDestroyed(QObject*)));

    if (pos != QPointF(-1, -1)) {
        applet->setPos(pos);
    }

    if (delayInit) {
        if (containmentType() == DesktopContainment) {
            applet->installSceneEventFilter(this);
            //applet->setWindowFlags(Qt::Window);
        }
    } else {
        applet->init();
        Animator::self()->animateItem(applet, Animator::AppearAnimation);
    }

    applet->updateConstraints(Plasma::AllConstraints);

    if (!currentContainment) {
        applet->updateConstraints(Plasma::StartupCompletedConstraint);
    }

    if (!delayInit) {
        applet->flushPendingConstraintsEvents();
    }

    emit appletAdded(applet, pos);
}

Applet::List Containment::applets() const
{
    return d->applets;
}

void Containment::setScreen(int screen)
{
    // What we want to do in here is:
    //   * claim the screen as our own
    //   * signal whatever may be watching this containment about the switch
    //   * if we are a full screen containment, then:
    //      * resize to match the screen if we're that kind of containment
    //      * kick other full-screen containments off this screen
    //          * if we had a screen, then give our screen to the containment
    //            we kick out
    //
    // a screen of -1 means no associated screen.
    Containment *swapScreensWith(0);
    if (d->type == DesktopContainment || d->type == CustomContainment) {
#ifndef Q_OS_WIN
        // we want to listen to changes in work area if our screen changes
        if (d->screen < 0 && screen > -1) {
            connect(KWindowSystem::self(), SIGNAL(workAreaChanged()),
                    this, SLOT(positionToolBox()));
        } else if (screen < 0) {
            disconnect(KWindowSystem::self(), SIGNAL(workAreaChanged()),
                       this, SLOT(positionToolBox()));
        }
#endif
        if (screen > -1 && corona()) {
            // sanity check to make sure someone else doesn't have this screen already!
            Containment *currently = corona()->containmentForScreen(screen);
            if (currently && currently != this) {
                //kDebug() << "currently is on screen" << currently->screen()
                //         << "and is" << currently->name()
                //         << (QObject*)currently << (QObject*)this;
                currently->setScreen(-1);
                swapScreensWith = currently;
            }
        }
    }

    //kDebug() << "setting screen to" << screen << "and we are a" << containmentType();
    Q_ASSERT(corona());
    int numScreens = corona()->numScreens();
    if (screen < -1) {
        screen = -1;
    }

    //kDebug() << "setting screen to " << screen << "and type is" << containmentType();
    if (screen < numScreens && screen > -1) {
        if (containmentType() == DesktopContainment ||
            containmentType() >= CustomContainment) {
            resize(corona()->screenGeometry(screen).size());
        }
    }

    int oldScreen = d->screen;
    d->screen = screen;
    updateConstraints(Plasma::ScreenConstraint);
    if (oldScreen != screen) {
        emit screenChanged(oldScreen, screen, this);

        KConfigGroup c = config();
        c.writeEntry("screen", d->screen);
        emit configNeedsSaving();
    }

    if (swapScreensWith) {
        swapScreensWith->setScreen(oldScreen);
    }
}

int Containment::screen() const
{
    return d->screen;
}

KPluginInfo::List Containment::listContainments(const QString &category,
                                                const QString &parentApp)
{
    QString constraint;

    if (parentApp.isEmpty()) {
        constraint.append("not exist [X-KDE-ParentApp]");
    } else {
        constraint.append("[X-KDE-ParentApp] == '").append(parentApp).append("'");
    }

    if (!category.isEmpty()) {
        if (!constraint.isEmpty()) {
            constraint.append(" and ");
        }

        constraint.append("[X-KDE-PluginInfo-Category] == '").append(category).append("'");
        if (category == "Miscellaneous") {
            constraint.append(" or (not exist [X-KDE-PluginInfo-Category] or [X-KDE-PluginInfo-Category] == '')");
        }
    }

    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Containment", constraint);
    //kDebug() << "constraint was" << constraint << "which got us" << offers.count() << "matches";
    return KPluginInfo::fromServices(offers);
}

KPluginInfo::List Containment::listContainmentsForMimetype(const QString &mimetype)
{
    QString constraint = QString("'%1' in [X-Plasma-DropMimeTypes]").arg(mimetype);
    //kDebug() << mimetype << constraint;
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Containment", constraint);
    return KPluginInfo::fromServices(offers);
}

void Containment::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    //kDebug() << immutability() << Mutable << (immutability() == Mutable);
    event->setAccepted(immutability() == Mutable &&
                       (event->mimeData()->hasFormat(static_cast<Corona*>(scene())->appletMimeType()) ||
                        KUrl::List::canDecode(event->mimeData())));

    if (!event->isAccepted()) {
        // check to see if we have an applet that accepts the format.
        QStringList formats = event->mimeData()->formats();

        foreach (const QString &format, formats) {
            KPluginInfo::List appletList = Applet::listAppletInfoForMimetype(format);
            if (!appletList.isEmpty()) {
                event->setAccepted(true);
                break;
            }
        }
    }
}

void Containment::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsItem *item = scene()->itemAt(event->scenePos());
    event->setAccepted(item == this || !item);
}

void Containment::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    //kDebug() << event->mimeData()->text();
    if (!isContainment()) {
        Applet::dropEvent(event);
        return;
    }

    QString mimetype(static_cast<Corona*>(scene())->appletMimeType());

    if (event->mimeData()->hasFormat(mimetype) && scene()) {
        QString data = event->mimeData()->data(mimetype);
        QStringList appletNames = data.split('\n', QString::SkipEmptyParts);

        foreach (const QString &appletName, appletNames) {
            //kDebug() << "doing" << appletName;
            QRectF geom(mapFromScene(event->scenePos()), QSize(0, 0));
            addApplet(appletName, QVariantList(), geom);
        }
        event->acceptProposedAction();
    } else if (KUrl::List::canDecode(event->mimeData())) {
        //TODO: collect the mimetypes of available script engines and offer
        //      to create widgets out of the matching URLs, if any
        KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        foreach (const KUrl &url, urls) {
            KMimeType::Ptr mime = KMimeType::findByUrl(url);
            QString mimeName = mime->name();
            QRectF geom(event->pos(), QSize());
            QVariantList args;
            args << url.url();
            //             kDebug() << mimeName;
            KPluginInfo::List appletList = Applet::listAppletInfoForMimetype(mimeName);

            if (!appletList.isEmpty()) {
                //TODO: should we show a dialog here to choose which plasmoid load if
                //!appletList.isEmpty()
                QMenu choices;
                QHash<QAction *, QString> actionsToPlugins;
                foreach (const KPluginInfo &info, appletList) {
                    QAction *action;
                    if (!info.icon().isEmpty()) {
                        action = choices.addAction(KIcon(info.icon()), info.name());
                    } else {
                        action = choices.addAction(info.name());
                    }

                    actionsToPlugins.insert(action, info.pluginName());
                }

                actionsToPlugins.insert(choices.addAction(i18n("Icon")), "icon");
                QAction *choice = choices.exec(event->screenPos());
                if (choice) {
                    addApplet(actionsToPlugins[choice], args, geom);
                }
            } else if (url.protocol() != "data") {
                // We don't try to do anything with data: URIs
                // no special applet associated with this mimetype, let's
                addApplet("icon", args, geom);
            }
        }
        event->acceptProposedAction();
    } else {
        QStringList formats = event->mimeData()->formats();
        QHash<QString, KPluginInfo> seenPlugins;
        QHash<QString, QString> pluginFormats;

        foreach (const QString &format, formats) {
            KPluginInfo::List plugins = Applet::listAppletInfoForMimetype(format);

            foreach (const KPluginInfo &plugin, plugins) {
                if (seenPlugins.contains(plugin.pluginName())) {
                    continue;
                }

                seenPlugins.insert(plugin.pluginName(), plugin);
                pluginFormats.insert(plugin.pluginName(), format);
            }
        }

        QString selectedPlugin;

        if (seenPlugins.isEmpty()) {
            // do nothing, we have no matches =/
        }

        if (seenPlugins.count() == 1) {
            selectedPlugin = seenPlugins.constBegin().key();
        } else {
            QMenu choices;
            QHash<QAction *, QString> actionsToPlugins;
            foreach (const KPluginInfo &info, seenPlugins) {
                QAction *action;
                if (!info.icon().isEmpty()) {
                    action = choices.addAction(KIcon(info.icon()), info.name());
                } else {
                    action = choices.addAction(info.name());
                }

                actionsToPlugins.insert(action, info.pluginName());
            }

            QAction *choice = choices.exec(event->screenPos());
            if (choice) {
                selectedPlugin = actionsToPlugins[choice];
            }
        }

        if (!selectedPlugin.isEmpty()) {
            KTemporaryFile tempFile;
            if (tempFile.open()) {
                //TODO: what should we do with files after the applet is done with them??
                tempFile.setAutoRemove(false);

                {
                    QDataStream stream(&tempFile);
                    QByteArray data = event->mimeData()->data(pluginFormats[selectedPlugin]);
                    stream.writeRawData(data, data.size());
                }

                QRectF geom(event->pos(), QSize());
                QVariantList args;
                args << tempFile.fileName();
                kDebug() << args;
                tempFile.close();

                addApplet(selectedPlugin, args, geom);
            }
        }
    }
}

void Containment::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    Applet::resizeEvent(event);
    if (d->wallpaper) {
        d->wallpaper->setBoundingRect(boundingRect());
    }
}

void Containment::keyPressEvent(QKeyEvent *event)
{
    //kDebug() << "keyPressEvent with" << event->key()
    //         << "and hoping and wishing for a" << Qt::Key_Tab;
    if (event->key() == Qt::Key_Tab) { // && event->modifiers() == 0) {
        if (!d->applets.isEmpty()) {
            kDebug() << "let's give focus to...." << (QObject*)d->applets.first();
            d->applets.first()->setFocus(Qt::TabFocusReason);
        }
    }
}

void Containment::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (d->wallpaper && d->wallpaper->isInitialized()) {
        QGraphicsItem *item = scene()->itemAt(event->scenePos());
        if (item == this) {
            event->ignore();
            d->wallpaper->wheelEvent(event);

            if (event->isAccepted()) {
                return;
            }

            event->accept();
        }
    }

    if (containmentType() == DesktopContainment) {
        QGraphicsItem *item = scene()->itemAt(event->scenePos());
        if (item == this) {
            int numDesktops = KWindowSystem::numberOfDesktops();
            int currentDesktop = KWindowSystem::currentDesktop();

            if (event->delta() < 0) {
                KWindowSystem::setCurrentDesktop(currentDesktop % numDesktops + 1);
            } else {
                KWindowSystem::setCurrentDesktop((numDesktops + currentDesktop - 2) % numDesktops + 1);
            }

            event->accept();
            return;
        }
    }

    event->ignore();
    Applet::wheelEvent(event);
}

bool Containment::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Applet *applet = qgraphicsitem_cast<Applet*>(watched);

    // Otherwise we're watching something we shouldn't be...
    Q_ASSERT(applet != 0);
    if (!d->applets.contains(applet)) {
        return false;
    }

    //kDebug() << "got sceneEvent";
    switch (event->type()) {
    case QEvent::GraphicsSceneHoverEnter:
        //kDebug() << "got hoverenterEvent" << immutability() << " " << applet->immutability();
        if (immutability() == Mutable && applet->immutability() == Mutable) {
            QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent*>(event);
            if (!d->handles.contains(applet)) {
                //kDebug() << "generated applet handle";
                AppletHandle *handle = new AppletHandle(this, applet, he->pos());
                d->handles[applet] = handle;
                connect(handle, SIGNAL(disappearDone(AppletHandle*)),
                        this, SLOT(handleDisappeared(AppletHandle*)));
                connect(applet, SIGNAL(geometryChanged()),
                        handle, SLOT(appletResized()));
            }
        }
        break;
    default:
        break;
    }

    return false;
}

QVariant Containment::itemChange(GraphicsItemChange change, const QVariant &value)
{
    //FIXME if the applet is moved to another containment we need to unfocus it

    if (isContainment() &&
        (change == QGraphicsItem::ItemSceneHasChanged ||
         change == QGraphicsItem::ItemPositionHasChanged) && !ContainmentPrivate::s_positioning) {
        switch (containmentType()) {
            case PanelContainment:
            case CustomPanelContainment:
                d->positionPanel();
                break;
            default:
                d->positionContainment();
                break;
        }
    }

    return Applet::itemChange(change, value);
}

void Containment::enableAction(const QString &name, bool enable)
{
    QAction *action = this->action(name);
    if (action) {
        action->setEnabled(enable);
        action->setVisible(enable);
    }
}

void Containment::addToolBoxAction(QAction *action)
{
    if (d->toolBox) {
        d->toolBox->addTool(action);
    }
}

void Containment::removeToolBoxAction(QAction *action)
{
    if (d->toolBox) {
        d->toolBox->removeTool(action);
    }
}

void Containment::setToolBoxOpen(bool open)
{
    if (open) {
        openToolBox();
    } else {
        closeToolBox();
    }
}

void Containment::openToolBox()
{
    if (d->toolBox) {
        d->toolBox->showToolBox();
    }
}

void Containment::closeToolBox()
{
    if (d->toolBox) {
        d->toolBox->hideToolBox();
    }
}

void Containment::addAssociatedWidget(QWidget *widget)
{
    Applet::addAssociatedWidget(widget);
    if (d->focusedApplet) {
        d->focusedApplet->addAssociatedWidget(widget);
    }

    foreach (const Applet *applet, d->applets) {
        if (applet->d->activationAction) {
            widget->addAction(applet->d->activationAction);
        }
    }
}

void Containment::removeAssociatedWidget(QWidget *widget)
{
    Applet::removeAssociatedWidget(widget);
    if (d->focusedApplet) {
        d->focusedApplet->removeAssociatedWidget(widget);
    }

    foreach (const Applet *applet, d->applets) {
        if (applet->d->activationAction) {
            widget->removeAction(applet->d->activationAction);
        }
    }
}

void Containment::setDrawWallpaper(bool drawWallpaper)
{
    d->drawWallpaper = drawWallpaper;
    if (drawWallpaper) {
        KConfigGroup cfg = config();
        QString wallpaper = cfg.readEntry("wallpaperplugin", defaultWallpaper);
        QString mode = cfg.readEntry("wallpaperpluginmode", defaultWallpaperMode);
        setWallpaper(wallpaper, mode);
    } else {
        delete d->wallpaper;
        d->wallpaper = 0;
    }
}

bool Containment::drawWallpaper()
{
    return d->drawWallpaper;
}

void Containment::setWallpaper(const QString &pluginName, const QString &mode)
{
    KConfigGroup cfg = config();
    bool newPlugin = true;
    bool newMode = true;

    if (d->drawWallpaper) {
        if (d->wallpaper) {
            // we have a wallpaper, so let's decide whether we need to swap it out
            if (d->wallpaper->pluginName() != pluginName) {
                delete d->wallpaper;
                d->wallpaper = 0;
            } else {
                // it's the same plugin, so let's save its state now so when
                // we call restore later on we're safe
                newMode = d->wallpaper->renderingMode().name() != mode;
                newPlugin = false;
            }
        }

        if (!pluginName.isEmpty() && !d->wallpaper) {
            d->wallpaper = Plasma::Wallpaper::load(pluginName);
        }

        if (d->wallpaper) {
            d->wallpaper->setBoundingRect(boundingRect());
            d->wallpaper->setRenderingMode(mode);

            if (newPlugin) {
                connect(d->wallpaper, SIGNAL(update(const QRectF&)),
                        this, SLOT(updateRect(const QRectF&)));
                cfg.writeEntry("wallpaperplugin", pluginName);
            }

            if (d->wallpaper->isInitialized()) {
                KConfigGroup wallpaperConfig = KConfigGroup(&cfg, "Wallpaper");
                wallpaperConfig = KConfigGroup(&wallpaperConfig, pluginName);
                d->wallpaper->restore(wallpaperConfig);
            }

            if (newMode) {
                cfg.writeEntry("wallpaperpluginmode", mode);
            }
        }

        update();
    }

    if (!d->wallpaper) {
        cfg.deleteEntry("wallpaperplugin");
        cfg.deleteEntry("wallpaperpluginmode");
    }

    if (newPlugin || newMode) {
        emit configNeedsSaving();
    }
}

Plasma::Wallpaper *Containment::wallpaper() const
{
    return d->wallpaper;
}

void Containment::setActivity(const QString &activity)
{
    Context *context = d->context();
    if (context->currentActivity() != activity) {
        context->setCurrentActivity(activity);

        foreach (Applet *a, d->applets) {
            a->updateConstraints(ContextConstraint);
        }

        KConfigGroup c = config();
        c.writeEntry("activity", activity);
        emit configNeedsSaving();
    }
}

QString Containment::activity() const
{
    return d->context()->currentActivity();
}

Context *ContainmentPrivate::context()
{
    if (!con) {
        con = new Context(q);
        q->connect(con, SIGNAL(changed(Plasma::Context*)),
                   q, SIGNAL(contextChanged(Plasma::Context*)));
    }

    return con;
}

KActionCollection &ContainmentPrivate::actions()
{
    return static_cast<Applet*>(q)->d->actions;
}

void ContainmentPrivate::focusApplet(Plasma::Applet *applet)
{
    if (focusedApplet == applet) {
        return;
    }

    QList<QWidget *> widgets = actions().associatedWidgets();
    if (focusedApplet) {
        foreach (QWidget *w, widgets) {
            focusedApplet->removeAssociatedWidget(w);
        }
    }
    //but what if applet isn't really one of our applets?
    //FIXME should we really unfocus the old applet?
    if (applet && applets.contains(applet)) {
        //kDebug() << "switching to" << applet->name();
        focusedApplet = applet;
        foreach (QWidget *w, widgets) {
            focusedApplet->addAssociatedWidget(w);
        }
        focusedApplet->setFocus(Qt::ShortcutFocusReason);
    } else {
        focusedApplet = 0;
    }
}

void Containment::focusNextApplet()
{
    if (d->applets.isEmpty()) {
        return;
    }
    int index = d->focusedApplet ? d->applets.indexOf(d->focusedApplet) + 1 : 0;
    if (index >= d->applets.size()) {
        index = 0;
    }
    kDebug() << "index" << index;
    d->focusApplet(d->applets.at(index));
}

void Containment::focusPreviousApplet()
{
    if (d->applets.isEmpty()) {
        return;
    }
    int index = d->focusedApplet ? d->applets.indexOf(d->focusedApplet) - 1 : -1;
    if (index < 0) {
        index = d->applets.size() - 1;
    }
    kDebug() << "index" << index;
    d->focusApplet(d->applets.at(index));
}

void Containment::destroy()
{
    destroy(true);
}

void Containment::destroy(bool confirm)
{
    if (immutability() != Mutable) {
        return;
    }

    if (isContainment()) {
        //don't remove a desktop that's in use
        //FIXME: this should probably be based on whether any views care or not!
        //       sth like: foreach (view) { view->requires(this); }
        Q_ASSERT(corona());
        if (d->type != PanelContainment && d->type != CustomPanelContainment &&
            (d->screen != -1 || d->screen >= corona()->numScreens())) {
            kDebug() << (QObject*)this << "containment has a screen number?" << d->screen;
            return;
        }

        //FIXME maybe that %1 should be the containment type not the name
        if (!confirm ||
            KMessageBox::warningContinueCancel(
                view(),
                i18nc("%1 is the name of the containment", "Do you really want to remove this %1?", name()),
                i18nc("@title:window %1 is the name of the containment", "Remove %1", name()), KStandardGuiItem::remove()) == KMessageBox::Continue) {
            //clearApplets();
            Applet::destroy();
        }
    } else {
        Applet::destroy();
    }
}

void Containment::showConfigurationInterface()
{
    if (isContainment()) {
        emit configureRequested(this);
    } else {
        Applet::showConfigurationInterface();
    }
}

// Private class implementation

void ContainmentPrivate::toggleDesktopImmutability()
{
    if (q->corona()) {
        if (q->corona()->immutability() == Mutable) {
            q->corona()->setImmutability(UserImmutable);
        } else if (q->corona()->immutability() == UserImmutable) {
            q->corona()->setImmutability(Mutable);
        }
    } else {
        if (q->immutability() == Mutable) {
            q->setImmutability(UserImmutable);
        } else if (q->immutability() == UserImmutable) {
            q->setImmutability(Mutable);
        }
    }

    if (q->immutability() != Mutable) {
        QMap<Applet*, AppletHandle*> h = handles;
        handles.clear();

        foreach (AppletHandle *handle, h) {
            handle->disconnect(q);
            handle->deleteLater();
        }
    }

    //setLockToolText();
}

void ContainmentPrivate::zoomIn()
{
    emit q->zoomRequested(q, Plasma::ZoomIn);
    positionToolBox();
}

void ContainmentPrivate::zoomOut()
{
    emit q->zoomRequested(q, Plasma::ZoomOut);
    positionToolBox();
}

ToolBox *ContainmentPrivate::createToolBox()
{
    if (!toolBox) {
        switch (type) {
        case Containment::PanelContainment:
            toolBox = new PanelToolBox(q);
            toolBox->setSize(22);
            toolBox->setIconSize(QSize(16, 16));
            if (q->immutability() != Mutable) {
                toolBox->hide();
            }
            break;
        case Containment::DesktopContainment:
            toolBox = new DesktopToolBox(q);
            break;
        default:
            break;
        }

        if (toolBox) {
            QObject::connect(toolBox, SIGNAL(toggled()), q, SIGNAL(toolBoxToggled()));
            positionToolBox();
        }
    }

    return toolBox;
}

void ContainmentPrivate::positionToolBox()
{
    if (!toolBox) {
        return;
    }

    //The placement assumes that the geometry width/height is no more than the screen
    if (type == Containment::PanelContainment) {
        if (q->formFactor() == Vertical) {
            toolBox->setCorner(ToolBox::Bottom);
            toolBox->setPos(q->geometry().width() / 2 - toolBox->boundingRect().width() / 2,
                            q->geometry().height());
        //defaulting to Horizontal right now
        } else {
            if (QApplication::layoutDirection() == Qt::RightToLeft) {
                toolBox->setPos(q->geometry().left(),
                                q->geometry().height() / 2 - toolBox->boundingRect().height() / 2);
                toolBox->setCorner(ToolBox::Left);
            } else {
                toolBox->setPos(q->geometry().width(),
                                q->geometry().height() / 2 - toolBox->boundingRect().height() / 2);
                toolBox->setCorner(ToolBox::Right);
            }
        }
    } else if (q->corona()) {
        //TODO: we should probably get these values from the Plasma app itself
        //      so we actually know what the available space *is*
        //      perhaps a virtual method in Corona for this?
        QRectF avail = q->corona()->availableScreenRegion(screen).boundingRect();
        QRectF screenGeom = q->corona()->screenGeometry(screen);

        // Transform to the containment's coordinate system.
        avail.translate(-screenGeom.topLeft());
        screenGeom.moveTo(0, 0);

        if (q->view() && !q->view()->transform().isScaling()) {
            if (QApplication::layoutDirection() == Qt::RightToLeft) {
                if (avail.top() > screenGeom.top()) {
                    toolBox->setPos(avail.topLeft() - QPoint(0, toolBox->size()));
                    toolBox->setCorner(ToolBox::Left);
                } else if (avail.left() > screenGeom.left()) {
                    toolBox->setPos(avail.topLeft() - QPoint(toolBox->size(), 0));
                    toolBox->setCorner(ToolBox::Top);
                } else {
                    toolBox->setPos(avail.topLeft());
                    toolBox->setCorner(ToolBox::TopLeft);
                }
            } else {
                if (avail.top() > screenGeom.top()) {
                    toolBox->setPos(avail.topRight() - QPoint(0, toolBox->size()));
                    toolBox->setCorner(ToolBox::Right);
                } else if (avail.right() < screenGeom.right()) {
                    toolBox->setPos(QPoint(avail.right() - toolBox->boundingRect().width(), avail.top()));
                    toolBox->setCorner(ToolBox::Top);
                } else {
                    toolBox->setPos(avail.topRight());
                    toolBox->setCorner(ToolBox::TopRight);
                }
            }
        } else {
            if (QApplication::layoutDirection() == Qt::RightToLeft) {
                toolBox->setPos(q->mapFromScene(QPointF(q->geometry().topLeft())));
                toolBox->setCorner(ToolBox::TopLeft);
            } else {
                toolBox->setPos(q->mapFromScene(QPointF(q->geometry().topRight())));
                toolBox->setCorner(ToolBox::TopRight);
            }
        }
    }
}

void ContainmentPrivate::triggerShowAddWidgets()
{
    emit q->showAddWidgetsInterface(QPointF());
}

void ContainmentPrivate::handleDisappeared(AppletHandle *handle)
{
    if (handles.contains(handle->applet())) {
        handles.remove(handle->applet());
        handle->detachApplet();
        handle->deleteLater();
    }
}

void ContainmentPrivate::containmentConstraintsEvent(Plasma::Constraints constraints)
{
    if (!q->isContainment()) {
        return;
    }

    //kDebug() << "got containmentConstraintsEvent" << constraints << (QObject*)toolBox;
    if (constraints & Plasma::ImmutableConstraint) {
        //update actions
        bool unlocked = q->immutability() == Mutable;
        q->setAcceptDrops(unlocked);

        QAction *action = actions().action("add widgets");
        if (action) {
            action->setVisible(unlocked);
            action->setEnabled(unlocked);
        }
        //FIXME immutability changes conflict with zoom changes
        /*action = actions().action("add sibling containment");
        if (action) {
            action->setVisible(unlocked);
            action->setEnabled(unlocked);
        }*/
        action = actions().action("lock widgets");
        if (action) {
            action->setText(unlocked ? i18n("Lock Widgets") : i18n("Unlock Widgets"));
            action->setIcon(KIcon(unlocked ? "object-locked" : "object-unlocked"));
        }

        // tell the applets too
        foreach (Applet *a, applets) {
            a->updateConstraints(ImmutableConstraint);
        }

        if (q->isContainment() && type == Containment::PanelContainment) {
            if (unlocked) {
                toolBox->show();
            } else {
                toolBox->hide();
            }
        }
    }

    if (constraints & Plasma::FormFactorConstraint) {
        if (toolBox) {
            if (q->formFactor() == Vertical) {
                toolBox->setCorner(ToolBox::Bottom);
                //defaults to horizontal
            } else if (QApplication::layoutDirection() == Qt::RightToLeft) {
                toolBox->setCorner(ToolBox::Left);
            } else {
                toolBox->setCorner(ToolBox::Right);
            }
        }

        foreach (Applet *applet, applets) {
            applet->updateConstraints(Plasma::FormFactorConstraint);
        }
    }

    if (constraints & Plasma::SizeConstraint && !ContainmentPrivate::s_positioning) {
        switch (q->containmentType()) {
            case Containment::PanelContainment:
            case Containment::CustomPanelContainment:
                positionPanel();
                break;
            default:
                positionContainment();
                break;
        }
    }

    if ((constraints & Plasma::SizeConstraint || constraints & Plasma::ScreenConstraint) &&
         toolBox) {
        positionToolBox();
    }
}

Applet *ContainmentPrivate::addApplet(const QString &name, const QVariantList &args,
                                      const QRectF &appletGeometry, uint id, bool delayInit)
{
    if (!q->isContainment()) {
        return 0;
    }

    if (!delayInit && q->immutability() != Mutable) {
        kDebug() << "addApplet for" << name << "requested, but we're currently immutable!";
        return 0;
    }

    QGraphicsView *v = q->view();
    if (v) {
        v->setCursor(Qt::BusyCursor);
    }

    Applet *applet = Applet::load(name, id, args);
    if (v) {
        v->unsetCursor();
    }

    if (!applet) {
        kDebug() << "Applet" << name << "could not be loaded.";
        applet = new Applet(0, QString(), id);
        applet->setFailedToLaunch(true, i18n("Could not find requested component: %1", name));
    }

    //kDebug() << applet->name() << "sizehint:" << applet->sizeHint() << "geometry:" << applet->geometry();

    q->addApplet(applet, appletGeometry.topLeft(), delayInit);
    return applet;
}

bool ContainmentPrivate::regionIsEmpty(const QRectF &region, Applet *ignoredApplet) const
{
    foreach (Applet *applet, applets) {
        if (applet != ignoredApplet && applet->geometry().intersects(region)) {
            return false;
        }
    }
    return true;
}

void ContainmentPrivate::appletDestroyed(QObject *object)
{
    // we do a static_cast here since it really isn't an Applet by this
    // point anymore since we are in the qobject dtor. we don't actually
    // try and do anything with it, we just need the value of the pointer
    // so this unsafe looking code is actually just fine.
    //
    // NOTE: DO NOT USE THE applet VARIABLE FOR ANYTHING OTHER THAN COMPARING
    //       THE ADDRESS! ACTUALLY USING THE OBJECT WILL RESULT IN A CRASH!!!
    Applet *applet = static_cast<Plasma::Applet*>(object);
    applets.removeAll(applet);
    if (focusedApplet == applet) {
        focusedApplet = 0;
    }

    if (handles.contains(applet)) {
        AppletHandle *handle = handles.value(applet);
        handles.remove(applet);
        handle->deleteLater();
    }

    emit q->appletRemoved(applet);
    emit q->configNeedsSaving();
}

void ContainmentPrivate::containmentAppletAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation anim)
{
    if (anim == Animator::AppearAnimation &&
        q->containmentType() == Containment::DesktopContainment &&
        item->parentItem() == q) {
        Applet *applet = qgraphicsitem_cast<Applet*>(item);

        if (applet) {
            applet->installSceneEventFilter(q);
            KConfigGroup *cg = applet->d->mainConfigGroup();
            applet->save(*cg);
            emit q->configNeedsSaving();
            //applet->setWindowFlags(Qt::Window);
        }
    }
}

bool containmentSortByPosition(const Containment *c1, const Containment *c2)
{
    return c1->id() < c2->id();
}

void ContainmentPrivate::positionContainment()
{
    Corona *c = q->corona();
    if (!c) {
        return;
    }

    //TODO: we should avoid running this too often; consider compressing requests
    //      with a timer.
    QList<Containment*> containments = c->containments();
    QMutableListIterator<Containment*> it(containments);

    while (it.hasNext()) {
        Containment *containment = it.next();
        if (containment->containmentType() == Containment::PanelContainment ||
            containment->containmentType() == Containment::CustomPanelContainment) {
            // weed out all containments we don't care about at all
            // e.g. Panels and ourself
            it.remove();
            continue;
        }
    }

    if (containments.isEmpty()) {
        return;
    }

    qSort(containments.begin(), containments.end(), containmentSortByPosition);
    it.toFront();

    int column = 0;
    int x = 0;
    int y = 0;
    int rowHeight = 0;
    //int count = 0;
    ContainmentPrivate::s_positioning = true;

    //kDebug() << "+++++++++++++++++++++++++++++++++++++++++++++++++++" << containments.count();
    while (it.hasNext()) {
        Containment *containment = it.next();
        containment->setPos(x, y);
        //kDebug() << ++count << "setting to" << x << y;

        int height = containment->size().height();
        if (height > rowHeight) {
            rowHeight = height;
        }

        ++column;

        if (column == CONTAINMENT_COLUMNS) {
            column = 0;
            x = 0;
            y += rowHeight + INTER_CONTAINMENT_MARGIN;
            rowHeight = 0;
        } else {
            x += containment->size().width() + INTER_CONTAINMENT_MARGIN;
        }
        //kDebug() << "column: " << column << "; x " << x << "; y" << y << "; width was"
        //         << containment->size().width();
    }
    //kDebug() << "+++++++++++++++++++++++++++++++++++++++++++++++++++";

    ContainmentPrivate::s_positioning = false;
}

void ContainmentPrivate::positionPanel(bool force)
{
    if (!q->scene()) {
        kDebug() << "no scene yet";
        return;
    }

    // we position panels in negative coordinates, and stack all horizontal
    // and all vertical panels with each other.

    const QPointF p = q->pos();

    if (!force &&
        p.y() + q->size().height() < -INTER_CONTAINMENT_MARGIN &&
        q->scene()->collidingItems(q).isEmpty()) {
        // already positioned and not running into any other panels
        return;
    }

    //TODO: research how non-Horizontal, non-Vertical (e.g. Planar) panels behave here
    bool horiz = q->formFactor() == Plasma::Horizontal;
    qreal bottom = horiz ? 0 : VERTICAL_STACKING_OFFSET;
    qreal lastHeight = 0;

    // this should be ok for small numbers of panels, but if we ever end
    // up managing hundreds of them, this simplistic alogrithm will
    // likely be too slow.
    foreach (const Containment *other, q->corona()->containments()) {
        if (other == q ||
            (other->containmentType() != Containment::PanelContainment &&
             other->containmentType() != Containment::CustomPanelContainment) ||
            horiz != (other->formFactor() == Plasma::Horizontal)) {
            // only line up with panels of the same orientation
            continue;
        }

        if (horiz) {
            qreal y = other->pos().y();
            if (y < bottom) {
                lastHeight = other->size().height();
                bottom = y;
            }
        } else {
            qreal width = other->size().width();
            qreal x = other->pos().x() + width;
            if (x > bottom) {
                lastHeight = width;
                bottom = x + lastHeight;
            }
        }
    }

    kDebug() << "positioning" << (horiz ? "" : "non-") << "horizontal panel; forced?" << force;
    // give a space equal to the height again of the last item so there is
    // room to grow.
    QPointF newPos;
    if (horiz) {
        bottom -= lastHeight + INTER_CONTAINMENT_MARGIN;
        //TODO: fix x position for non-flush-left panels
        kDebug() << "moved to" << QPointF(0, bottom - q->size().height());
        newPos = QPointF(0, bottom - q->size().height());
    } else {
        bottom += lastHeight + INTER_CONTAINMENT_MARGIN;
        //TODO: fix y position for non-flush-top panels
        kDebug() << "moved to" << QPointF(bottom + q->size().width(), -INTER_CONTAINMENT_MARGIN - q->size().height());
        newPos = QPointF(bottom + q->size().width(), -INTER_CONTAINMENT_MARGIN - q->size().height());
    }

    ContainmentPrivate::s_positioning = true;
    if (p != newPos) {
        q->setPos(newPos);
        emit q->geometryChanged();
    }
    ContainmentPrivate::s_positioning = false;
}

} // Plasma namespace

#include "containment.moc"

