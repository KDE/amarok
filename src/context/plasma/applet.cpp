/*
 *   Copyright 2005 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
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

#include "applet.h"
#include "private/applet_p.h"

#include <cmath>
#include <limits>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QGraphicsGridLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QList>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QSize>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>
#include <QUiLoader>

#include <KAction>
#include <KIcon>
#include <KColorScheme>
#include <KConfigDialog>
#include <KDialog>
#include <KIconLoader>
#include <KPluginInfo>
#include <KStandardDirs>
#include <KService>
#include <KServiceTypeTrader>
#include <KShortcut>
#include <KWindowSystem>
#include <KActionCollection>
#include <KAuthorized>

#include <Solid/PowerManagement>

#include "configloader.h"
#include "containment.h"
#include "corona.h"
#include "dataenginemanager.h"
#include "extender.h"
#include "extenderitem.h"
#include "package.h"
#include "plasma.h"
#include "scripting/appletscript.h"
#include "svg.h"
#include "framesvg.h"
#include "popupapplet.h"
#include "theme.h"
#include "view.h"
#include "widgets/iconwidget.h"
#include "widgets/label.h"
#include "widgets/pushbutton.h"
#include "tooltipmanager.h"
#include "wallpaper.h"

#include "private/containment_p.h"
#include "private/extenderapplet_p.h"
#include "private/packages_p.h"
#include "private/popupapplet_p.h"
#include "private/toolbox_p.h"

//#define DYNAMIC_SHADOWS
namespace Plasma
{

Applet::Applet(QGraphicsItem *parent,
               const QString &serviceID,
               uint appletId)
    :  QGraphicsWidget(parent),
       d(new AppletPrivate(KService::serviceByStorageId(serviceID), appletId, this))
{
    // WARNING: do not access config() OR globalConfig() in this method!
    //          that requires a scene, which is not available at this point
    d->init();
}

Applet::Applet(QObject *parentObject, const QVariantList &args)
    :  QGraphicsWidget(0),
       d(new AppletPrivate(
             KService::serviceByStorageId(args.count() > 0 ? args[0].toString() : QString()),
             args.count() > 1 ? args[1].toInt() : 0, this))
{
    // now remove those first two items since those are managed by Applet and subclasses shouldn't
    // need to worry about them. yes, it violates the constness of this var, but it lets us add
    // or remove items later while applets can just pretend that their args always start at 0
    QVariantList &mutableArgs = const_cast<QVariantList &>(args);
    if (!mutableArgs.isEmpty()) {
        mutableArgs.removeFirst();

        if (!mutableArgs.isEmpty()) {
            mutableArgs.removeFirst();
        }
    }

    setParent(parentObject);
    // WARNING: do not access config() OR globalConfig() in this method!
    //          that requires a scene, which is not available at this point
    d->init();

    // the brain damage seen in the initialization list is due to the
    // inflexibility of KService::createInstance
}

Applet::~Applet()
{
    if (d->extender) {
        //This would probably be nicer if it was located in extender. But in it's dtor, this won't
        //work since when that get's called, the applet's config() isn't accessible anymore. (same
        //problem with calling saveState(). Doing this in saveState() might be a possibility, but
        //that would require every extender savestate implementation to call it's parent function,
        //which isn't very nice.
        foreach (ExtenderItem *item, d->extender->attachedItems()) {
            if (!item->isDetached() || item->autoExpireDelay()) {
                //destroy temporary extender items, or items that aren't detached, so their
                //configuration won't linger after a plasma restart.
                item->destroy();
            }
        }

        d->extender->saveState();
    }

    if (d->transient) {
        d->resetConfigurationObject();
    }

    delete d;
}

PackageStructure::Ptr Applet::packageStructure()
{
    if (!AppletPrivate::packageStructure) {
        AppletPrivate::packageStructure = new PlasmoidPackage();
    }

    return AppletPrivate::packageStructure;
}

void Applet::init()
{
    if (d->script && !d->script->init()) {
        setFailedToLaunch(true, i18n("Script initialization failed"));
    }
}

uint Applet::id() const
{
    return d->appletId;
}

void Applet::save(KConfigGroup &g) const
{
    KConfigGroup group = g;
    if (!group.isValid()) {
        group = *d->mainConfigGroup();
    }

    kDebug() << "saving to" << group.name();
    // we call the dptr member directly for locked since isImmutable()
    // also checks kiosk and parent containers
    group.writeEntry("immutability", (int)d->immutability);
    group.writeEntry("plugin", pluginName());
    //FIXME: for containments, we need to have some special values here w/regards to
    //       screen affinity (e.g. "bottom of screen 0")
    //kDebug() << pluginName() << "geometry is" << geometry()
    //         << "pos is" << pos() << "bounding rect is" << boundingRect();
    group.writeEntry("geometry", geometry());
    group.writeEntry("zvalue", zValue());

    if (transform() == QTransform()) {
        group.deleteEntry("transform");
    } else {
        QList<qreal> m;
        QTransform t = transform();
        m << t.m11() << t.m12() << t.m13() << t.m21() << t.m22() << t.m23() << t.m31() << t.m32() << t.m33();
        group.writeEntry("transform", m);
        //group.writeEntry("transform", transformToString(transform()));
    }

    KConfigGroup appletConfigGroup(&group, "Configuration");

    //FIXME: we need a global save state too
    saveState(appletConfigGroup);

    if (d->activationAction) {
        KConfigGroup shortcutConfig(&group, "Shortcuts");
        shortcutConfig.writeEntry("global", d->activationAction->globalShortcut().toString());
    }
}

void Applet::restore(KConfigGroup &group)
{
    QList<qreal> m = group.readEntry("transform", QList<qreal>());
    if (m.count() == 9) {
        QTransform t(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
        setTransform(t);
    }

    qreal z = group.readEntry("zvalue", 0);

    if (z >= AppletPrivate::s_maxZValue) {
        AppletPrivate::s_maxZValue = z;
    }

    if (z > 0) {
        setZValue(z);
    }

    setImmutability((ImmutabilityType)group.readEntry("immutability", (int)Mutable));

    QRectF geom = group.readEntry("geometry", QRectF());
    if (geom.isValid()) {
        setGeometry(geom);
    }

    KConfigGroup shortcutConfig(&group, "Shortcuts");
    QString shortcutText = shortcutConfig.readEntryUntranslated("global", QString());
    if (!shortcutText.isEmpty()) {
        setGlobalShortcut(KShortcut(shortcutText));
        kDebug() << "got global shortcut for" << name() << "of" << QKeySequence(shortcutText);
        kDebug() << "set to" << d->activationAction->objectName()
                 << d->activationAction->globalShortcut().primary();
    }

    // local shortcut, if any
    //TODO: implement; the shortcut will need to be registered with the containment
    /*
    shortcutText = shortcutConfig.readEntryUntranslated("local", QString());
    if (!shortcutText.isEmpty()) {
        //TODO: implement; the shortcut
    }
    */
    // start up is done, we can now go do a mod timer
    if (d->modificationsTimerId > 0) {
        killTimer(d->modificationsTimerId);
    }
    d->modificationsTimerId = 0;
}

void AppletPrivate::setFocus()
{
    kDebug() << "setting focus";
    q->setFocus(Qt::ShortcutFocusReason);
}

void Applet::setFailedToLaunch(bool failed, const QString &reason)
{
    if (d->failed == failed) {
        return;
    }

    d->failed = failed;
    prepareGeometryChange();

    qDeleteAll(QGraphicsItem::children());
    setLayout(0);

    if (failed) {
        setBackgroundHints(d->backgroundHints|StandardBackground);

        QGraphicsLinearLayout *failureLayout = new QGraphicsLinearLayout(this);
        failureLayout->setContentsMargins(0, 0, 0, 0);

        IconWidget *failureIcon = new IconWidget(this);
        failureIcon->setIcon(KIcon("dialog-error"));
        failureLayout->addItem(failureIcon);

        Label *failureWidget = new Plasma::Label(this);
        failureWidget->setText(d->visibleFailureText(reason));
        QLabel *label = failureWidget->nativeWidget();
        label->setWordWrap(true);
        failureLayout->addItem(failureWidget);

        Plasma::ToolTipManager::self()->registerWidget(failureIcon);
        Plasma::ToolTipContent data(i18n("Unable to load the widget"), reason,
                                    KIcon("dialog-error").pixmap(IconSize(KIconLoader::Desktop)));
        Plasma::ToolTipManager::self()->setContent(failureIcon, data);

        setLayout(failureLayout);
        resize(300, 250);
        setMinimumSize(failureLayout->minimumSize());
        d->background->resizeFrame(geometry().size());

    }
    update();
}

void Applet::saveState(KConfigGroup &group) const
{
    if (group.config()->name() != config().config()->name()) {
        // we're being saved to a different file!
        // let's just copy the current values in our configuration over
        KConfigGroup c = config();
        c.copyTo(&group);
    }
}

KConfigGroup Applet::config(const QString &group) const
{
    KConfigGroup cg = config();
    return KConfigGroup(&cg, group);
}

KConfigGroup Applet::config() const
{
    if (d->isContainment) {
        return *(d->mainConfigGroup());
    }

    return KConfigGroup(d->mainConfigGroup(), "Configuration");
}

KConfigGroup Applet::globalConfig() const
{
    KConfigGroup globalAppletConfig;
    const Containment *c = containment();
    QString group = isContainment() ? "ContainmentGlobals" : "AppletGlobals";

    if (c && c->corona()) {
        KSharedConfig::Ptr coronaConfig = c->corona()->config();
        globalAppletConfig = KConfigGroup(coronaConfig, group);
    } else {
        globalAppletConfig = KConfigGroup(KGlobal::config(), group);
    }

    return KConfigGroup(&globalAppletConfig, d->globalName());
}

void Applet::destroy()
{
    if (immutability() != Mutable || d->transient) {
        return; //don't double delete
    }

    d->transient = true;

    if (isContainment()) {
        d->cleanUpAndDelete();
    } else {
        connect(Animator::self(), SIGNAL(animationFinished(QGraphicsItem*,Plasma::Animator::Animation)),
                this, SLOT(appletAnimationComplete(QGraphicsItem*,Plasma::Animator::Animation)));
        Animator::self()->animateItem(this, Animator::DisappearAnimation);
    }
}

void AppletPrivate::appletAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation anim)
{
    if (anim != Animator::DisappearAnimation || item != q) {
        return; //it's not our time yet
    }

    cleanUpAndDelete();
}

void AppletPrivate::selectItemToDestroy()
{
    //FIXME: this will not work nicely with multiple screens and being zoomed out!
    if (q->isContainment()) {
        QGraphicsView *view = q->view();
        if (view && view->transform().isScaling() &&
            q->scene()->focusItem() != q) {
            QGraphicsItem *focus = q->scene()->focusItem();

            if (focus) {
                Containment *toDestroy = dynamic_cast<Containment*>(focus->topLevelItem());

                if (toDestroy) {
                    toDestroy->destroy();
                    return;
                }
            }
        }
    }

    q->destroy();
}

void AppletPrivate::updateRect(const QRectF &rect)
{
    q->update(rect);
}

void AppletPrivate::cleanUpAndDelete()
{
    //kDebug() << "???????????????? DESTROYING APPLET" << name() << " ???????????????????????????";
    QGraphicsWidget *parent = dynamic_cast<QGraphicsWidget *>(q->parentItem());
    //it probably won't matter, but right now if there are applethandles, *they* are the parent.
    //not the containment.

    //is the applet in a containment and is the containment have a layout?
    //if yes, we remove the applet in the layout
    if (parent && parent->layout()) {
        QGraphicsLayout *l = parent->layout();
        for (int i = 0; i < l->count(); ++i) {
            if (q == l->itemAt(i)) {
                l->removeAt(i);
                break;
            }
        }
    }

    if (configLoader) {
        configLoader->setDefaults();
    }

    q->scene()->removeItem(q);
    q->deleteLater();
}

ConfigLoader *Applet::configScheme() const
{
    return d->configLoader;
}

DataEngine *Applet::dataEngine(const QString &name) const
{
    int index = d->loadedEngines.indexOf(name);
    if (index != -1) {
        return DataEngineManager::self()->engine(name);
    }

    DataEngine *engine = DataEngineManager::self()->loadEngine(name);
    if (engine->isValid()) {
        d->loadedEngines.append(name);
    }

    return engine;
}

const Package *Applet::package() const
{
    return d->package;
}

QGraphicsView *Applet::view() const
{
    // It's assumed that we won't be visible on more than one view here.
    // Anything that actually needs view() should only really care about
    // one of them anyway though.
    if (!scene()) {
        return 0;
    }

    QGraphicsView *found = 0;
    QGraphicsView *possibleFind = 0;
    //kDebug() << "looking through" << scene()->views().count() << "views";
    foreach (QGraphicsView *view, scene()->views()) {
        //kDebug() << "     checking" << view << view->sceneRect()
        //         << "against" << sceneBoundingRect() << scenePos();
        if (view->sceneRect().intersects(sceneBoundingRect()) ||
            view->sceneRect().contains(scenePos())) {
            //kDebug() << "     found something!" << view->isActiveWindow();
            if (view->isActiveWindow()) {
                found = view;
            } else {
                possibleFind = view;
            }
        }
    }

    return found ? found : possibleFind;
}

QRectF Applet::mapFromView(const QGraphicsView *view, const QRect &rect) const
{
    // Why is this adjustment needed? Qt calculation error?
    return mapFromScene(view->mapToScene(rect)).boundingRect().adjusted(0, 0, 1, 1);
}

QRect Applet::mapToView(const QGraphicsView *view, const QRectF &rect) const
{
    // Why is this adjustment needed? Qt calculation error?
    return view->mapFromScene(mapToScene(rect)).boundingRect().adjusted(0, 0, -1, -1);
}

QPoint Applet::popupPosition(const QSize &s) const
{
    Q_ASSERT(containment());
    Q_ASSERT(containment()->corona());
    return containment()->corona()->popupPosition(this, s);
}

void Applet::updateConstraints(Plasma::Constraints constraints)
{
    d->scheduleConstraintsUpdate(constraints);
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    //NOTE: do NOT put any code in here that reacts to constraints updates
    //      as it will not get called for any applet that reimplements constraintsEvent
    //      without calling the Applet:: version as well, which it shouldn't need to.
    //      INSTEAD put such code into flushPendingConstraintsEvents
    Q_UNUSED(constraints)
    //kDebug() << constraints << "constraints are FormFactor: " << formFactor()
    //         << ", Location: " << location();
    if (d->script) {
        d->script->constraintsEvent(constraints);
    }
}

void Applet::initExtenderItem(ExtenderItem *item)
{
    Q_UNUSED(item)
    item->destroy();
}

Extender *Applet::extender() const
{
    if (!d->extender) {
        new Extender(const_cast<Applet*>(this));
    }

    return d->extender;
}

QString Applet::name() const
{
    if (isContainment()) {
        if (!d->appletDescription.isValid()) {
            return i18n("Unknown Activity");
        }

        const Containment *c = qobject_cast<const Containment*>(this);
        if (c && !c->activity().isNull()) {
            return i18n("%1 Activity", c->activity());
        }
    } else if (!d->appletDescription.isValid()) {
        return i18n("Unknown Widget");
    }

    return d->appletDescription.name();
}

QFont Applet::font() const
{
    return QApplication::font();
}

QString Applet::icon() const
{
    if (!d->appletDescription.isValid()) {
        return QString();
    }

    return d->appletDescription.icon();
}

QString Applet::pluginName() const
{
    if (!d->appletDescription.isValid()) {
        return QString();
    }

    return d->appletDescription.pluginName();
}

bool Applet::shouldConserveResources() const
{
    return Solid::PowerManagement::appShouldConserveResources();
}

QString Applet::category() const
{
    if (!d->appletDescription.isValid()) {
        return i18n("Miscellaneous");
    }

    return d->appletDescription.category();
}

QString Applet::category(const KPluginInfo &applet)
{
    return applet.property("X-KDE-PluginInfo-Category").toString();
}

QString Applet::category(const QString &appletName)
{
    if (appletName.isEmpty()) {
        return QString();
    }

    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(appletName);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Applet", constraint);

    if (offers.isEmpty()) {
        return QString();
    }

    return offers.first()->property("X-KDE-PluginInfo-Category").toString();
}

ImmutabilityType Applet::immutability() const
{
    //Returning the more strict immutability between the applet immutability and Corona one
    ImmutabilityType coronaImmutability = Mutable;

    if (dynamic_cast<Corona*>(scene())) {
        coronaImmutability = static_cast<Corona*>(scene())->immutability();
    }

    if (coronaImmutability == SystemImmutable) {
        return SystemImmutable;
    } else if (coronaImmutability == UserImmutable && d->immutability != SystemImmutable) {
        return UserImmutable;
    } else {
        return d->immutability;
    }
}

void Applet::setImmutability(const ImmutabilityType immutable)
{
    if (d->immutability == immutable) {
        return;
    }

    d->immutability = immutable;
    updateConstraints(ImmutableConstraint);
}

Applet::BackgroundHints Applet::backgroundHints() const
{
    return d->backgroundHints;
}

void Applet::setBackgroundHints(const BackgroundHints hints)
{
    d->backgroundHints = hints;

    //Draw the standard background?
    if ((hints & StandardBackground) || (hints & TranslucentBackground)) {
        if (!d->background) {
            d->background = new Plasma::FrameSvg(this);
        }

        if ((hints & TranslucentBackground) &&
            Plasma::Theme::defaultTheme()->currentThemeHasImage("widgets/translucentbackground")) {
            d->background->setImagePath("widgets/translucentbackground");
        } else {
            d->background->setImagePath("widgets/background");
        }

        d->background->setEnabledBorders(Plasma::FrameSvg::AllBorders);
        qreal left, top, right, bottom;
        d->background->getMargins(left, top, right, bottom);
        setContentsMargins(left, right, top, bottom);
        QSizeF fitSize(left + right, top + bottom);
        if (minimumSize().expandedTo(fitSize) != minimumSize()) {
            setMinimumSize(minimumSize().expandedTo(fitSize));
        }
        d->background->resizeFrame(boundingRect().size());
    } else if (d->background) {
        qreal left, top, right, bottom;
        d->background->getMargins(left, top, right, bottom);
        //Setting a minimum size of 0,0 would result in the panel to be only
        //on the first virtual desktop
        setMinimumSize(qMax(minimumSize().width() - left - right, qreal(1.0)),
                       qMax(minimumSize().height() - top - bottom, qreal(1.0)));

        delete d->background;
        d->background = 0;
        setContentsMargins(0, 0, 0, 0);
    }
}

bool Applet::hasFailedToLaunch() const
{
    return d->failed;
}

void Applet::paintWindowFrame(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
    //Here come the code for the window frame
    //kDebug() << windowFrameGeometry();
    //painter->drawRoundedRect(windowFrameGeometry(), 5, 5);
}

bool Applet::configurationRequired() const
{
    return d->needsConfigOverlay != 0;
}

void Applet::setConfigurationRequired(bool needsConfig, const QString &reason)
{
    if ((d->needsConfigOverlay != 0) == needsConfig) {
        return;
    }

    if (d->needsConfigOverlay) {
        QGraphicsWidget *w = d->needsConfigOverlay;
        d->needsConfigOverlay = 0;
        w->hide();
        w->deleteLater();
        return;
    }

    d->needsConfigOverlay = new AppletOverlayWidget(this);
    d->needsConfigOverlay->resize(contentsRect().size());
    d->needsConfigOverlay->setPos(contentsRect().topLeft());

    int zValue = 100;
    foreach (QGraphicsItem *child, QGraphicsItem::children()) {
        if (child->zValue() > zValue) {
            zValue = child->zValue() + 1;
        }
    }
    d->needsConfigOverlay->setZValue(zValue);

    qDeleteAll(d->needsConfigOverlay->QGraphicsItem::children());
    QGraphicsGridLayout *configLayout = new QGraphicsGridLayout(d->needsConfigOverlay);
    configLayout->setContentsMargins(0, 0, 0, 0);

  //  configLayout->addStretch();
    configLayout->setColumnStretchFactor(0, 10);
    configLayout->setColumnStretchFactor(2, 10);
    configLayout->setRowStretchFactor(0, 10);
    configLayout->setRowStretchFactor(3, 10);

    int row = 1;
    if (!reason.isEmpty()) {
        Label *explanation = new Label(d->needsConfigOverlay);
        explanation->setText(reason);
        configLayout->addItem(explanation, row, 1);
        configLayout->setColumnStretchFactor(1, 10);
        ++row;
        //configLayout->setAlignment(explanation, Qt::AlignBottom | Qt::AlignCenter);
    }

    PushButton *configWidget = new PushButton(d->needsConfigOverlay);
    configWidget->setText(i18n("Configure..."));
    connect(configWidget, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()));
    configLayout->addItem(configWidget, row, 1);
    //configLayout->setAlignment(configWidget, Qt::AlignTop | Qt::AlignCenter);
    //configLayout->addStretch();

    d->needsConfigOverlay->show();
}

void Applet::flushPendingConstraintsEvents()
{
    if (d->pendingConstraints == NoConstraint) {
        return;
    }

    if (d->constraintsTimerId) {
        killTimer(d->constraintsTimerId);
        d->constraintsTimerId = 0;
    }

    //kDebug() << "fushing constraints: " << d->pendingConstraints << "!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    Plasma::Constraints c = d->pendingConstraints;
    d->pendingConstraints = NoConstraint;

    if (c & Plasma::StartupCompletedConstraint) {
        //common actions
        bool unlocked = immutability() == Mutable;
        //FIXME desktop containments can't be removed while in use.
        //it's kinda silly to have a keyboard shortcut for something that can only be used when the
        //shortcut isn't active.
        QAction *closeApplet = new QAction(this);
        closeApplet->setIcon(KIcon("edit-delete"));
        closeApplet->setEnabled(unlocked);
        closeApplet->setVisible(unlocked);
        closeApplet->setShortcutContext(Qt::WidgetShortcut); //don't clash with other views
        closeApplet->setText(i18nc("%1 is the name of the applet", "Remove this %1", name()));
        if (isContainment()) {
            closeApplet->setShortcut(QKeySequence("ctrl+shift+r"));
        } else {
            closeApplet->setShortcut(QKeySequence("ctrl+r"));
        }
        connect(closeApplet, SIGNAL(triggered(bool)), this, SLOT(selectItemToDestroy()));
        d->actions.addAction("remove", closeApplet);
    }

    if (c & Plasma::ImmutableConstraint) {
        bool unlocked = immutability() == Mutable;
        QAction *action = d->actions.action("remove");
        if (action) {
            action->setVisible(unlocked);
            action->setEnabled(unlocked);
        }
        if (!KAuthorized::authorize("PlasmaAllowConfigureWhenLocked")) {
            action = d->actions.action("configure");
            if (action) {
                action->setVisible(unlocked);
                action->setEnabled(unlocked);
            }
        }
    }

    if (c & Plasma::SizeConstraint && d->needsConfigOverlay) {
        d->needsConfigOverlay->setGeometry(QRectF(QPointF(0, 0), geometry().size()));

        QGraphicsItem *button = 0;
        QList<QGraphicsItem*> children = d->needsConfigOverlay->QGraphicsItem::children();

        if (!children.isEmpty()) {
            button = children.first();
        }

        if (button) {
            QSizeF s = button->boundingRect().size();
            button->setPos(d->needsConfigOverlay->boundingRect().width() / 2 - s.width() / 2,
                           d->needsConfigOverlay->boundingRect().height() / 2 - s.height() / 2);
        }
    }

    if (c & Plasma::FormFactorConstraint) {
        FormFactor f = formFactor();
        if (!isContainment() && f != Vertical && f != Horizontal) {
            setBackgroundHints(d->backgroundHints | StandardBackground);
        } else if(d->backgroundHints & StandardBackground) {
            setBackgroundHints(d->backgroundHints ^ StandardBackground);
        } else if(d->backgroundHints & TranslucentBackground) {
            setBackgroundHints(d->backgroundHints ^ TranslucentBackground);
        }

        if (d->failed) {
            if (f == Vertical || f == Horizontal) {
                setMinimumSize(0, 0);
                QGraphicsLayoutItem *item = layout()->itemAt(1);
                layout()->removeAt(1);
                delete item;
            }
        }
    }

    //enforce square size in panels
    if ((c & Plasma::SizeConstraint || c & Plasma::FormFactorConstraint) &&
        aspectRatioMode() == Plasma::Square) {
        if (formFactor() == Horizontal) {
            setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        } else if (formFactor() == Vertical) {
            setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
        }

        updateGeometry();
    }

    //enforce a constrained square size in panels
    if ((c & Plasma::SizeConstraint || c & Plasma::FormFactorConstraint) &&
        aspectRatioMode() == Plasma::ConstrainedSquare) {
        if (formFactor() == Horizontal) {
            setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        } else if (formFactor() == Vertical) {
            setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
        }

        updateGeometry();
    }

    Containment* containment = qobject_cast<Plasma::Containment*>(this);
    if (isContainment() && containment) {
        containment->d->containmentConstraintsEvent(c);
    }

    PopupApplet* popup = qobject_cast<Plasma::PopupApplet*>(this);
    if (popup) {
        popup->d->popupConstraintsEvent(c);
    }

    constraintsEvent(c);

    if (layout()) {
        layout()->updateGeometry();
    }
}

int Applet::type() const
{
    return Type;
}

QList<QAction*> Applet::contextualActions()
{
    //kDebug() << "empty context actions";
    return d->script ? d->script->contextualActions() : QList<QAction*>();
}

QAction *Applet::action(QString name) const
{
    return d->actions.action(name);
}

void Applet::addAction(QString name, QAction *action)
{
    d->actions.addAction(name, action);
}

void Applet::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPainter *p;
    //FIXME: we should probably set the pixmap to screenSize(), but that breaks stuff atm.
    QPixmap *pixmap = 0;

    if (d->ghost) {
        // The applet has to be displayed semi transparent. Create a pixmap and a painter on
        // that pixmap where the applet can draw on so we can draw the result transparently
        // at the end.
        kDebug() << "Painting ghosted...";
        pixmap = new QPixmap(boundingRect().size().toSize());
        pixmap->fill(Qt::transparent);

        p = new QPainter();
        p->begin(pixmap);
    } else {
        p = painter;
    }

    p->save();

    if (transform().isRotating()) {
        p->setRenderHint(QPainter::SmoothPixmapTransform);
        p->setRenderHint(QPainter::Antialiasing);
    }

    if (d->background &&
        formFactor() != Plasma::Vertical &&
        formFactor() != Plasma::Horizontal) {
        //kDebug() << "option rect is" << option->rect;
        d->background->paintFrame(p);
    }

    if (!d->failed) {
        qreal left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect contentsRect =
            QRectF(QPointF(0, 0),
                   boundingRect().size()).adjusted(left, top, -right, -bottom).toRect();

        if (widget && isContainment()) {
            // note that the widget we get is actually the viewport of the view, not the view itself
            View* v = qobject_cast<Plasma::View*>(widget->parent());

            if (!v || v->isWallpaperEnabled()) {
                Containment* c = qobject_cast<Plasma::Containment*>(this);
                if (c && c->drawWallpaper() && c->wallpaper()) {
                    Wallpaper *w = c->wallpaper();
                    if (!w->isInitialized()) {
                        // delayed paper initialization
                        KConfigGroup wallpaperConfig = c->config();
                        wallpaperConfig = KConfigGroup(&wallpaperConfig, "Wallpaper");
                        wallpaperConfig = KConfigGroup(&wallpaperConfig, w->pluginName());
                        w->restore(wallpaperConfig);
                    }

                    p->save();
                    c->wallpaper()->paint(p, option->exposedRect);
                    p->restore();
                }

                Containment::StyleOption coption(*option);
                coption.view = v;
                paintInterface(p, &coption, contentsRect);
            }

            p->restore();
            return;
        }

        //kDebug() << "paint interface of" << (QObject*) this;
        paintInterface(p, option, contentsRect);
    }
    p->restore();

    if (d->ghost) {
        // Lets display the pixmap that we've just drawn... transparently.
        p->setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p->fillRect(pixmap->rect(), QColor(0, 0, 0, (0.3 * 255)));
        p->end();
        delete p;

        painter->drawPixmap(0, 0, *pixmap);
        delete pixmap;
    }
}

void Applet::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option,
                            const QRect &contentsRect)
{
    if (d->script) {
        d->script->paintInterface(painter, option, contentsRect);
    } else {
        //kDebug() << "Applet::paintInterface() default impl";
    }
}

FormFactor Applet::formFactor() const
{
    Containment *c = containment();
    return c ? c->d->formFactor : Plasma::Planar;
}

Containment *Applet::containment() const
{
    if (isContainment()) {
        Containment *c = dynamic_cast<Containment*>(const_cast<Applet*>(this));
        if (c) {
            return c;
        }
    }

    QGraphicsItem *parent = parentItem();
    Containment *c = 0;

    while (parent) {
        Containment *possibleC = dynamic_cast<Containment*>(parent);
        if (possibleC && possibleC->isContainment()) {
            c = possibleC;
            break;
        }
        parent = parent->parentItem();
    }

    return c;
}

void Applet::setGlobalShortcut(const KShortcut &shortcut)
{
    if (!d->activationAction) {
        d->activationAction = new KAction(this);
        d->activationAction->setText(i18n("Activate %1 Widget", name()));
        d->activationAction->setObjectName(QString("activate widget %1").arg(id())); // NO I18N
        connect(d->activationAction, SIGNAL(triggered()), this, SIGNAL(activate()));
        connect(this, SIGNAL(activate()), this, SLOT(setFocus()));

        QList<QWidget *> widgets = d->actions.associatedWidgets();
        foreach (QWidget *w, widgets) {
            w->addAction(d->activationAction);
        }
    }

    //kDebug() << "before" << shortcut.primary() << d->activationAction->globalShortcut().primary();
    d->activationAction->setGlobalShortcut(
        shortcut,
        KAction::ShortcutTypes(KAction::ActiveShortcut | KAction::DefaultShortcut),
        KAction::NoAutoloading);
    //kDebug() << "after" << shortcut.primary() << d->activationAction->globalShortcut().primary();
}

KShortcut Applet::globalShortcut() const
{
    if (d->activationAction) {
        return d->activationAction->globalShortcut();
    }

    return KShortcut();
}

void Applet::addAssociatedWidget(QWidget *widget)
{
    d->actions.addAssociatedWidget(widget);
}

void Applet::removeAssociatedWidget(QWidget *widget)
{
    d->actions.removeAssociatedWidget(widget);
}

Location Applet::location() const
{
    Containment *c = containment();
    return c ? c->d->location : Plasma::Desktop;
}

Context *Applet::context() const
{
    Containment *c = containment();
    Q_ASSERT(c);
    return c->d->context();
}

Plasma::AspectRatioMode Applet::aspectRatioMode() const
{
    return d->aspectRatioMode;
}

void Applet::setAspectRatioMode(Plasma::AspectRatioMode mode)
{
    d->aspectRatioMode = mode;
}

void Applet::registerAsDragHandle(QGraphicsItem *item)
{
    if (!item) {
        return;
    }

    int index = d->registeredAsDragHandle.indexOf(item);

    if (index == -1) {
        d->registeredAsDragHandle.append(item);
        item->installSceneEventFilter(this);
    }
}

void Applet::unregisterAsDragHandle(QGraphicsItem *item)
{
    if (!item) {
        return;
    }

    int index = d->registeredAsDragHandle.indexOf(item);
    if (index != -1) {
        d->registeredAsDragHandle.removeAt(index);
        item->removeSceneEventFilter(this);
    }
}

bool Applet::isRegisteredAsDragHandle(QGraphicsItem *item)
{
    return (d->registeredAsDragHandle.indexOf(item) != -1);
}

bool Applet::hasConfigurationInterface() const
{
    return d->hasConfigurationInterface;
}

void Applet::setHasConfigurationInterface(bool hasInterface)
{
    if (d->hasConfigurationInterface == hasInterface) {
        return;
    }

    d->hasConfigurationInterface = hasInterface;
    //config action
    //TODO respect security when it's implemented (4.2)
    QAction *configAction = d->actions.action("configure");
    if (hasInterface) {
        if (!configAction) { //should be always true
            configAction = new QAction(i18n("%1 Settings", name()), this);
            configAction->setIcon(KIcon("configure"));
            configAction->setShortcutContext(Qt::WidgetShortcut); //don't clash with other views
            if (isContainment()) {
                //kDebug() << "I am a containment";
                configAction->setShortcut(QKeySequence("ctrl+shift+s"));
            } else {
                configAction->setShortcut(QKeySequence("ctrl+s"));
            }
            //TODO how can we handle configuration of the shortcut in a way that spans all applets?
            connect(configAction, SIGNAL(triggered(bool)),
                    this, SLOT(showConfigurationInterface()));
            d->actions.addAction("configure", configAction);
        }
    } else {
        d->actions.removeAction(configAction);
    }
}

bool Applet::eventFilter(QObject *o, QEvent *e)
{
    return QObject::eventFilter(o, e);
}

bool Applet::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::GraphicsSceneMouseMove:
    {
        // don't move when the containment is not mutable,
        // in the rare case the containment doesn't exists consider it as mutable
        if ((!containment() || containment()->immutability() == Mutable) &&
            d->registeredAsDragHandle.contains(watched)) {
            mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
            return true;
        }
        break;
    }

    default:
        break;
    }

    return QGraphicsItem::sceneEventFilter(watched, event);
}

void Applet::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (immutability() == Mutable && formFactor() == Plasma::Planar) {
        QGraphicsItem *parent = parentItem();
        Plasma::Applet *applet = qgraphicsitem_cast<Plasma::Applet*>(parent);

        if (applet && applet->isContainment()) {
            // our direct parent is a containment. just move ourselves.
            QPointF curPos = event->pos();
            QPointF lastPos = event->lastPos();
            QPointF delta = curPos - lastPos;

            moveBy(delta.x(), delta.y());
        } else if (parent) {
            //don't move the icon as well because our parent
            //(usually an appletHandle) will do it for us
            //parent->moveBy(delta.x(),delta.y());
            QPointF curPos = parent->transform().map(event->pos());
            QPointF lastPos = parent->transform().map(event->lastPos());
            QPointF delta = curPos - lastPos;

            parent->setPos(parent->pos() + delta);
        }
    }
}

void Applet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    QGraphicsWidget::mousePressEvent(event);
}

void Applet::focusInEvent(QFocusEvent *event)
{
    if (!isContainment() && containment()) {
        //focusing an applet may trigger this event again, but we won't be here more than twice
        containment()->d->focusApplet(this);
    }

    QGraphicsWidget::focusInEvent(event);
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);

    if (d->background) {
        d->background->resizeFrame(boundingRect().size());
    }

    updateConstraints(Plasma::SizeConstraint);

    if (d->modificationsTimerId != -1) {
        // schedule a save
        if (d->modificationsTimerId) {
            killTimer(d->modificationsTimerId);
        }
        d->modificationsTimerId = startTimer(1000);
    }

    emit geometryChanged();
}

void Applet::showConfigurationInterface()
{
    if (!hasConfigurationInterface()) {
        return;
    }

    if (immutability() != Mutable && !KAuthorized::authorize("PlasmaAllowConfigureWhenLocked")) {
        return;
    }

    const QString dialogId = QString("%1settings%2").arg(id()).arg(name());
    KConfigDialog * dlg = KConfigDialog::exists(dialogId);

    if (dlg) {
        KWindowSystem::setOnDesktop(dlg->winId(), KWindowSystem::currentDesktop());
        dlg->show();
        KWindowSystem::activateWindow(dlg->winId());
        return;
    }

    const QString windowTitle = i18nc("@title:window", "%1 Settings", name());
    if (d->package && d->configLoader) {
        QString uiFile = d->package->filePath("mainconfigui");
        if (uiFile.isEmpty()) {
            return;
        }

        KConfigDialog *dialog = new KConfigDialog(0, dialogId, d->configLoader);
        dialog->setWindowTitle(windowTitle);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);

        QUiLoader loader;
        QFile f(uiFile);
        if (!f.open(QIODevice::ReadOnly)) {
            delete dialog;

            if (d->script) {
                d->script->showConfigurationInterface();
            }
            return;
        }

        QWidget *w = loader.load(&f);
        f.close();

        dialog->addPage(w, i18n("Settings"), icon(), i18n("%1 Settings", name()));
        dialog->show();
    } else if (d->script) {
        d->script->showConfigurationInterface();
    } else {
        KConfigSkeleton *nullManager = new KConfigSkeleton(0);
        KConfigDialog *dialog = new KConfigDialog(0, dialogId, nullManager);
        dialog->setFaceType(KPageDialog::Auto);
        dialog->setWindowTitle(windowTitle);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        createConfigurationInterface(dialog);
        //TODO: would be nice to not show dialog if there are no pages added?
        connect(dialog, SIGNAL(finished()), nullManager, SLOT(deleteLater()));
        //TODO: Apply button does not correctly work for now, so do not show it
        dialog->showButton(KDialog::Apply, false);
        connect(dialog, SIGNAL(applyClicked()), this, SLOT(configChanged()));
        connect(dialog, SIGNAL(okClicked()), this, SLOT(configChanged()));
        dialog->show();
    }

    emit releaseVisualFocus();
}

void Applet::configChanged()
{
    if (d->script) {
        d->script->configChanged();
    }
}

void Applet::createConfigurationInterface(KConfigDialog *parent)
{
    Q_UNUSED(parent)
    // virtual method reimplemented by subclasses.
    // do not put anything here ...
}

KPluginInfo::List Applet::listAppletInfo(const QString &category,
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

    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Applet", constraint);
    //kDebug() << "Applet::listAppletInfo constraint was '" << constraint
    //         << "' which got us " << offers.count() << " matches";
    return KPluginInfo::fromServices(offers);
}

KPluginInfo::List Applet::listAppletInfoForMimetype(const QString &mimetype)
{
    QString constraint = QString("'%1' in [X-Plasma-DropMimeTypes]").arg(mimetype);
    //kDebug() << "listAppletInfoForMimetype with" << mimetype << constraint;
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Applet", constraint);
    return KPluginInfo::fromServices(offers);
}

QStringList Applet::listCategories(const QString &parentApp, bool visibleOnly)
{
    QString constraint = "exist [X-KDE-PluginInfo-Category]";

    if (parentApp.isEmpty()) {
        constraint.append(" and not exist [X-KDE-ParentApp]");
    } else {
        constraint.append(" and [X-KDE-ParentApp] == '").append(parentApp).append("'");
    }

    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Applet", constraint);
    QStringList categories;
    foreach (const KService::Ptr &applet, offers) {
        QString appletCategory = applet->property("X-KDE-PluginInfo-Category").toString();
        if (visibleOnly && applet->noDisplay()) {
            // we don't want to show the hidden category
            continue;
        }

        //kDebug() << "   and we have " << appletCategory;
        if (appletCategory.isEmpty()) {
            if (!categories.contains(i18n("Miscellaneous"))) {
                categories << i18n("Miscellaneous");
            }
        } else  if (!categories.contains(appletCategory)) {
            categories << appletCategory;
        }
    }

    categories.sort();
    return categories;
}

Applet *Applet::load(const QString &appletName, uint appletId, const QVariantList &args)
{
    if (appletName.isEmpty()) {
        return 0;
    }

    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(appletName);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Applet", constraint);

    bool isContainment = false;
    if (offers.isEmpty()) {
        //TODO: what would be -really- cool is offer to try and download the applet
        //      from the network at this point
        offers = KServiceTypeTrader::self()->query("Plasma/Containment", constraint);
        isContainment = true;
        if (offers.isEmpty()) {
            kDebug() << "offers is empty for " << appletName;
            return 0;
        }
    } /* else if (offers.count() > 1) {
        kDebug() << "hey! we got more than one! let's blindly take the first one";
    } */

    KService::Ptr offer = offers.first();

    if (appletId == 0) {
        appletId = ++AppletPrivate::s_maxAppletId;
    }

    if (!offer->property("X-Plasma-API").toString().isEmpty()) {
        kDebug() << "we have a script using the"
                 << offer->property("X-Plasma-API").toString() << "API";
        if (isContainment) {
            return new Containment(0, offer->storageId(), appletId);
        }
        return new Applet(0, offer->storageId(), appletId);
    }

    KPluginLoader plugin(*offer);

    if (!Plasma::isPluginVersionCompatible(plugin.pluginVersion()) &&
        (appletName != "internal:extender")) {
        return 0;
    }

    QVariantList allArgs;
    allArgs << offer->storageId() << appletId << args;
    QString error;
    Applet *applet;

    if (appletName == "internal:extender") {
        applet = new ExtenderApplet(0, allArgs);
    } else {
        applet = offer->createInstance<Plasma::Applet>(0, allArgs, &error);
    }

    if (!applet) {
        kDebug() << "Couldn't load applet \"" << appletName << "\"! reason given: " << error;
    }

    return applet;
}

Applet *Applet::load(const KPluginInfo &info, uint appletId, const QVariantList &args)
{
    if (!info.isValid()) {
        return 0;
    }

    return load(info.pluginName(), appletId, args);
}

QVariant Applet::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant ret = QGraphicsWidget::itemChange(change, value);

    //kDebug() << change;
    switch (change) {
    case ItemSceneHasChanged:
    {
        QGraphicsScene *newScene = qvariant_cast<QGraphicsScene*>(value);
        if (newScene) {
            d->checkImmutability();
        }
    }
    break;
    case ItemPositionHasChanged:
        emit geometryChanged();
        // fall through!
    case ItemTransformHasChanged:
    {
        if (d->modificationsTimerId != -1) {
            if (d->modificationsTimerId) {
                killTimer(d->modificationsTimerId);
            }
            d->modificationsTimerId = startTimer(1000);
        }
    }
    break;
    default:
        break;
    };

    return ret;
}

QPainterPath Applet::shape() const
{
    if (d->script) {
        return d->script->shape();
    }

    return QGraphicsWidget::shape();
}

QSizeF Applet::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    QSizeF hint = QGraphicsWidget::sizeHint(which, constraint);

    //in panels make sure that the contents won't exit from the panel
    if (formFactor() == Horizontal && which == Qt::MinimumSize) {
        hint.setHeight(0);
    } else if (formFactor() == Vertical && which == Qt::MinimumSize) {
        hint.setWidth(0);
    }

    // enforce a square size in panels
    if (d->aspectRatioMode == Plasma::Square) {
        if (formFactor() == Horizontal) {
            hint.setWidth(size().height());
        } else if (formFactor() == Vertical) {
            hint.setHeight(size().width());
        }
    } else if (d->aspectRatioMode == Plasma::ConstrainedSquare) {
        //enforce a size not wider than tall
        if (formFactor() == Horizontal &&
            (which == Qt::MaximumSize || size().height() <= KIconLoader::SizeLarge)) {
            hint.setWidth(size().height());
        //enforce a size not taller than wide
        } else if (formFactor() == Vertical &&
                   (which == Qt::MaximumSize || size().width() <= KIconLoader::SizeLarge)) {
            hint.setHeight(size().width());
        }
    }

    return hint;
}

void Applet::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
}

void Applet::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
}

void Applet::timerEvent(QTimerEvent *event)
{
    if (d->transient) {
        killTimer(d->constraintsTimerId);
        killTimer(d->modificationsTimerId);
        return;
    }

    if (event->timerId() == d->constraintsTimerId) {
        killTimer(d->constraintsTimerId);
        d->constraintsTimerId = 0;
        flushPendingConstraintsEvents();
    } else if (event->timerId() == d->modificationsTimerId) {
        killTimer(d->modificationsTimerId);
        d->modificationsTimerId = 0;
        // invalid group, will result in save using the default group
        KConfigGroup cg;
        save(cg);
        emit configNeedsSaving();
    }
}

QRect Applet::screenRect() const
{
    QGraphicsView *v = view();

    if (v) {
        QPointF bottomRight = pos();
        bottomRight.rx() += size().width();
        bottomRight.ry() += size().height();

        QPoint tL = v->mapToGlobal(v->mapFromScene(pos()));
        QPoint bR = v->mapToGlobal(v->mapFromScene(bottomRight));
        return QRect(QPoint(tL.x(), tL.y()), QSize(bR.x() - tL.x(), bR.y() - tL.y()));
    }

    //The applet doesn't have a view on it.
    //So a screenRect isn't relevant.
    return QRect(QPoint(0, 0), QSize(0, 0));
}

void Applet::raise()
{
    setZValue(++AppletPrivate::s_maxZValue);
}

void Applet::lower()
{
    setZValue(--AppletPrivate::s_minZValue);
}

void AppletPrivate::setIsContainment(bool nowIsContainment)
{
    if (isContainment == nowIsContainment) {
        return;
    }

    isContainment = nowIsContainment;

    Containment *c = qobject_cast<Containment*>(q);
    if (c) {
        if (isContainment) {
            // set up the toolbox
            c->d->createToolBox();
        } else {
            delete c->d->toolBox;
            c->d->toolBox = 0;
        }
    }
}

bool Applet::isContainment() const
{
    return d->isContainment;
}

// PRIVATE CLASS IMPLEMENTATION

AppletPrivate::AppletPrivate(KService::Ptr service, int uniqueID, Applet *applet)
        : appletId(uniqueID),
          q(applet),
          extender(0),
          backgroundHints(Applet::StandardBackground),
          appletDescription(service),
          needsConfigOverlay(0),
          background(0),
          script(0),
          package(0),
          configLoader(0),
          mainConfig(0),
          pendingConstraints(NoConstraint),
          aspectRatioMode(Plasma::KeepAspectRatio),
          immutability(Mutable),
          actions(applet),
          activationAction(0),
          constraintsTimerId(0),
          modificationsTimerId(-1),
          hasConfigurationInterface(false),
          failed(false),
          isContainment(false),
          transient(false),
          ghost(false)
{
    if (appletId == 0) {
        appletId = ++s_maxAppletId;
    } else if (appletId > s_maxAppletId) {
        s_maxAppletId = appletId;
    }
}

AppletPrivate::~AppletPrivate()
{
    modificationsTimerId = -1;

    if (activationAction && activationAction->isGlobalShortcutEnabled()) {
        //kDebug() << "reseting global action for" << q->name() << activationAction->objectName();
        activationAction->forgetGlobalShortcut();
    }

    foreach (const QString &engine, loadedEngines) {
        DataEngineManager::self()->unloadEngine(engine);
    }

    if (extender) {
        delete extender;
        extender = 0;
    }

    delete script;
    script = 0;
    delete package;
    package = 0;
    delete configLoader;
    configLoader = 0;
    delete mainConfig;
    mainConfig = 0;
}

void AppletPrivate::init()
{
    // WARNING: do not access config() OR globalConfig() in this method!
    //          that requires a scene, which is not available at this point
    q->setCacheMode(Applet::DeviceCoordinateCache);
    q->setAcceptsHoverEvents(true);
    q->setFlag(QGraphicsItem::ItemIsFocusable, true);
    // FIXME: adding here because nothing seems to be doing it in QGraphicsView,
    // but it doesn't actually work anyways =/
    q->setLayoutDirection(qApp->layoutDirection());

    if (!appletDescription.isValid()) {
        kDebug() << "Check your constructor! "
                 << "You probably want to be passing in a Service::Ptr "
                 << "or a QVariantList with a valid storageid as arg[0].";
        return;
    }

    QString api = appletDescription.property("X-Plasma-API").toString();

    // we have a scripted plasmoid
    if (!api.isEmpty()) {
        // find where the Package is
        QString path = KStandardDirs::locate(
            "data",
            "plasma/plasmoids/" + appletDescription.pluginName() + '/');

        if (path.isEmpty()) {
            q->setFailedToLaunch(
                true,
                i18nc("Package file, name of the widget",
                      "Could not locate the %1 package required for the %2 widget.")
                      .arg(appletDescription.pluginName(), appletDescription.name()));
        } else {
            // create the package and see if we have something real
            //kDebug() << "trying for" << path;
            PackageStructure::Ptr structure =
                Plasma::packageStructure(api, Plasma::AppletComponent);
            structure->setPath(path);
            package = new Package(path, structure);

            if (package->isValid()) {
                // now we try and set up the script engine.
                // it will be parented to this applet and so will get
                // deleted when the applet does

                script = Plasma::loadScriptEngine(api, q);
                if (!script) {
                    delete package;
                    package = 0;
                    q->setFailedToLaunch(true,
                                         i18nc("API or programming language the widget was written in, name of the widget",
                                               "Could not create a %1 ScriptEngine for the %2 widget.")
                                               .arg(api, appletDescription.name()));
                }
            } else {
                q->setFailedToLaunch(true, i18nc("Package file, name of the widget",
                                                 "Could not open the %1 package required for the %2 widget.")
                                                 .arg(appletDescription.pluginName(), appletDescription.name()));
                delete package;
                package = 0;
            }

            if (package) {
                setupScriptSupport();
            }
        }
    }

    q->setBackgroundHints(Applet::DefaultBackground);

    QObject::connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), q, SLOT(themeChanged()));
}

// put all setup routines for script here. at this point we can assume that
// package exists and that we have a script engine
void AppletPrivate::setupScriptSupport()
{
    Q_ASSERT(package);
    QString xmlPath = package->filePath("mainconfigxml");
    if (!xmlPath.isEmpty()) {
        QFile file(xmlPath);
        // FIXME: KConfigSkeleton doesn't play well with KConfigGroup =/
        KConfigGroup config = q->config();
        configLoader = new ConfigLoader(&config, &file);
        QObject::connect(configLoader, SIGNAL(configChanged()), q, SLOT(configChanged()));
    }

    if (!package->filePath("mainconfigui").isEmpty()) {
        q->setHasConfigurationInterface(true);
    }
}

QString AppletPrivate::globalName() const
{
    if (!appletDescription.isValid()) {
        return QString();
    }

    return appletDescription.service()->library();
}

QString AppletPrivate::instanceName()
{
    if (!appletDescription.isValid()) {
        return QString();
    }

    return appletDescription.service()->library() + QString::number(appletId);
}

void AppletPrivate::scheduleConstraintsUpdate(Plasma::Constraints c)
{
    // Don't start up a timer if we're just starting up
    // flushPendingConstraints will be called by Corona
    if (!constraintsTimerId && !(c & Plasma::StartupCompletedConstraint)) {
        constraintsTimerId = q->startTimer(0);
    }

    pendingConstraints |= c;
}

KConfigGroup *AppletPrivate::mainConfigGroup()
{
    if (mainConfig) {
        return mainConfig;
    }

    if (isContainment) {
        const Containment *asContainment = qobject_cast<Containment*>(const_cast<Applet*>(q));
        Q_ASSERT(asContainment);

        KConfigGroup containmentConfig;
        //kDebug() << "got a corona, baby?" << (QObject*)asContainment->corona();
        if (asContainment->corona()) {
            containmentConfig = KConfigGroup(asContainment->corona()->config(), "Containments");
        } else {
            containmentConfig =  KConfigGroup(KGlobal::config(), "Containments");
        }

        mainConfig = new KConfigGroup(&containmentConfig, QString::number(appletId));
    } else {
        KConfigGroup appletConfig;
        if (q->containment()) {
            appletConfig = q->containment()->config();
            appletConfig = KConfigGroup(&appletConfig, "Applets");
        } else {
            kWarning() << "requesting config for" << q->name() << "without a containment!";
            appletConfig = KConfigGroup(KGlobal::config(), "Applets");
        }

        mainConfig = new KConfigGroup(&appletConfig, QString::number(appletId));
    }

    return mainConfig;
}

QString AppletPrivate::visibleFailureText(const QString &reason)
{
    QString text;

    if (reason.isEmpty()) {
        text = i18n("This object could not be created.");
    } else {
        text = i18n("This object could not be created for the following reason:<p><b>%1</b></p>", reason);
    }

    return text;
}

void AppletPrivate::checkImmutability()
{
    const bool systemImmutable = q->globalConfig().isImmutable() || q->config().isImmutable() ||
                                ((!isContainment && q->containment()) &&
                                    q->containment()->immutability() == SystemImmutable) ||
                                (dynamic_cast<Corona*>(q->scene()) && static_cast<Corona*>(q->scene())->immutability() == SystemImmutable);

    if (systemImmutable) {
        q->updateConstraints(ImmutableConstraint);
    }
}

void AppletPrivate::themeChanged()
{
    if (background) {
        //do again the translucent background fallback
        q->setBackgroundHints(backgroundHints);

        qreal left;
        qreal right;
        qreal top;
        qreal bottom;
        background->getMargins(left, top, right, bottom);
        q->setContentsMargins(left, right, top, bottom);
    }
    q->update();
}

void AppletPrivate::resetConfigurationObject()
{
    mainConfigGroup()->deleteGroup();
    delete mainConfig;
    mainConfig = 0;
}

uint AppletPrivate::s_maxAppletId = 0;
uint AppletPrivate::s_maxZValue = 0;
uint AppletPrivate::s_minZValue = 0;
PackageStructure::Ptr AppletPrivate::packageStructure(0);

AppletOverlayWidget::AppletOverlayWidget(QGraphicsWidget *parent)
    : QGraphicsWidget(parent)
{
    resize(parent->size());
}

void AppletOverlayWidget::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem *option,
                                QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QColor wash = Plasma::Theme::defaultTheme()->color(Theme::BackgroundColor);
    wash.setAlphaF(.6);

    Applet *applet = qobject_cast<Applet *>(parentWidget());

    if (applet->backgroundHints() & Applet::StandardBackground) {
        painter->fillRect(parentWidget()->contentsRect(), wash);
    } else {
        painter->fillPath(parentItem()->shape(), wash);
    }

    painter->restore();
}

} // Plasma namespace

#include "applet.moc"
