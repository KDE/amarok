/*
 * Copyright 2007 by Dan Meltzer <hydrogen@notyetimplemented.com>
 * Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 * Copyright 2008 by Alexis Ménard <darktears31@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "tooltipmanager.h"

//Qt
#include <QLabel>
#include <QTimer>
#include <QGridLayout>
#include <QGraphicsView>

//KDE
#include <KWindowSystem>

//X11
#ifdef Q_WS_X11
#include <QtGui/QX11Info>
#include <X11/Xlib.h>
#include <fixx11h.h>
#endif

//Plasma
#include <applet.h>
#include <containment.h>
#include <corona.h>
#include <framesvg.h>
#include <theme.h>
#include <view.h>
#include <private/tooltip_p.h>

namespace Plasma
{

class ToolTipManagerPrivate
{
public :
    ToolTipManagerPrivate()
        : currentWidget(0),
          showTimer(0),
          hideTimer(0),
          state(ToolTipManager::Activated),
          isShown(false),
          delayedHide(false)
    {

    }

    ~ToolTipManagerPrivate()
    {
        clearTips();
    }

    void showToolTip();
    void resetShownState();

    /**
      * called when the theme of plasma has change
      */
    void themeUpdated();
    /**
      * called when a widget inside the tooltip manager is deleted
      */
    void onWidgetDestroyed(QObject * object);

    void clearTips();

    void doDelayedHide();

    QGraphicsWidget *currentWidget;
    QTimer *showTimer;
    QTimer *hideTimer;
    QHash<QGraphicsWidget *, ToolTip *> tooltips;
    ToolTipManager::State state;
    bool isShown : 1;
    bool delayedHide : 1;
};

//TOOLTIP IMPLEMENTATION
class ToolTipManagerSingleton
{
    public:
    ToolTipManagerSingleton()
    {
    }
    ToolTipManager self;
};
K_GLOBAL_STATIC(ToolTipManagerSingleton, privateInstance)

ToolTipManager *ToolTipManager::self()
{
    return &privateInstance->self;
}

ToolTipManager::Content::Content()
    : windowToPreview(0),
      autohide(true)
{
}

bool ToolTipManager::Content::isEmpty() const
{
    return mainText.isEmpty() && subText.isEmpty() && image.isNull() && windowToPreview == 0;
}

ToolTipManager::ToolTipManager(QObject *parent)
  : QObject(parent),
    d(new ToolTipManagerPrivate)
{
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeUpdated()));
    d->themeUpdated();

    d->showTimer = new QTimer(this);
    d->showTimer->setSingleShot(true);
    d->hideTimer = new QTimer(this);
    d->hideTimer->setSingleShot(true);

    connect(d->showTimer, SIGNAL(timeout()), SLOT(showToolTip()));
    connect(d->hideTimer, SIGNAL(timeout()), SLOT(resetShownState()));
}

ToolTipManager::~ToolTipManager()
{
    delete d;
}

void ToolTipManager::show(QGraphicsWidget *widget)
{
    if (!d->tooltips.contains(widget)) {
        return;
    }

    if (d->currentWidget) {
        hide(d->currentWidget);
    }

    d->hideTimer->stop();
    d->delayedHide = false;
    d->showTimer->stop();
    d->currentWidget = widget;

    if (d->isShown) {
        // small delay to prevent unnecessary showing when the mouse is moving quickly across items
        // which can be too much for less powerful CPUs to keep up with
        d->showTimer->start(200);
    } else {
        d->showTimer->start(700);
    }
}

bool ToolTipManager::isVisible(QGraphicsWidget *widget) const
{
    ToolTip *tooltip = d->tooltips.value(widget);
    if (tooltip) {
        return tooltip->isVisible();
    } else {
        return false;
    }
}

void ToolTipManagerPrivate::doDelayedHide()
{
    showTimer->stop();  // stop the timer to show the tooltip
    delayedHide = true;
    hideTimer->start(250);
}

void ToolTipManager::hide(QGraphicsWidget *widget)
{
    ToolTip *tooltip = d->tooltips.value(widget);
    if (tooltip) {
        d->showTimer->stop();  // stop the timer to show the tooltip
        d->delayedHide = false;
        d->currentWidget = 0;
        tooltip->hide();
    }
}

void ToolTipManager::registerWidget(QGraphicsWidget *widget)
{
    if (d->state == Deactivated || d->tooltips.contains(widget)) {
        return;
    }

    //the tooltip is not registered we add it in our map of tooltips
    d->tooltips.insert(widget, 0);
    widget->installEventFilter(this);
    //connect to object destruction
    connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(onWidgetDestroyed(QObject*)));
}

void ToolTipManager::unregisterWidget(QGraphicsWidget *widget)
{
    if (!d->tooltips.contains(widget)) {
        return;
    }

    widget->removeEventFilter(this);
    ToolTip *tooltip = d->tooltips.take(widget);
    if (tooltip) {
        tooltip->deleteLater();
    }
}

void ToolTipManager::setContent(QGraphicsWidget *widget, const Content &data)
{
    if (d->state == Deactivated) {
        return;
    }

    registerWidget(widget);

    ToolTip *tooltip = d->tooltips.value(widget);

    if (data.isEmpty()) {
        if (tooltip) {
            tooltip->deleteLater();
        }
        d->tooltips.insert(widget, 0);
        return;
    }

    if (!tooltip) {
        tooltip = new ToolTip(widget);
        d->tooltips.insert(widget, tooltip);
    }

    tooltip->setContent(data);
    tooltip->updateTheme();
}

void ToolTipManager::clearContent(QGraphicsWidget *widget)
{
    setContent(widget, Content());
}

void ToolTipManager::setState(ToolTipManager::State state)
{
    d->state = state;

    switch (state) {
        case Activated:
            break;
        case Deactivated:
            d->clearTips();
            //fallthrough
        case Inhibited:
            d->resetShownState();
            break;
    }
}

ToolTipManager::State ToolTipManager::state() const
{
    return d->state;
}

void ToolTipManagerPrivate::themeUpdated()
{
    QHashIterator<QGraphicsWidget*, ToolTip *> iterator(tooltips);
    while (iterator.hasNext()) {
        iterator.next();
        if (iterator.value()) {
            iterator.value()->updateTheme();
        }
    }
}

void ToolTipManagerPrivate::onWidgetDestroyed(QObject *object)
{
    if (!object) {
        return;
    }

    // we do a static_cast here since it really isn't a QGraphicsWidget by this
    // point anymore since we are in the QObject dtor. we don't actually
    // try and do anything with it, we just need the value of the pointer
    // so this unsafe looking code is actually just fine.
    //
    // NOTE: DO NOT USE THE w VARIABLE FOR ANYTHING OTHER THAN COMPARING
    //       THE ADDRESS! ACTUALLY USING THE OBJECT WILL RESULT IN A CRASH!!!
    QGraphicsWidget *w = static_cast<QGraphicsWidget*>(object);

    if (currentWidget == w) {
        currentWidget = 0;
        showTimer->stop();  // stop the timer to show the tooltip
        delayedHide = false;
    }

    QMutableHashIterator<QGraphicsWidget*, ToolTip *> iterator(tooltips);
    while (iterator.hasNext()) {
        iterator.next();
        //kDebug() << (int)iterator.key() << (int)w;
        if (iterator.key() == w) {
            ToolTip *tooltip = iterator.value();
            iterator.remove();
            if (tooltip) {
                //kDebug() << "deleting the tooltip!";
                tooltip->hide();
                tooltip->deleteLater();
            }
            return;
        }
    }
}

void ToolTipManagerPrivate::clearTips()
{
    foreach (ToolTip *tip, tooltips) {
        delete tip;
    }

    tooltips.clear();
}

void ToolTipManagerPrivate::resetShownState()
{
    if (currentWidget) {
        ToolTip * tooltip = tooltips.value(currentWidget);
        if (tooltip && (!tooltip->isVisible() || delayedHide)) {
            //One might have moused out and back in again
            delayedHide = false;
            isShown = false;
            tooltip->hide();
            currentWidget = 0;
        }
    }
}

void ToolTipManagerPrivate::showToolTip()
{
    if (state != ToolTipManager::Activated ||
        !currentWidget ||
        QApplication::activePopupWidget() ||
        QApplication::activeModalWidget()) {
        return;
    }

    ToolTip *tooltip = tooltips.value(currentWidget);
    bool justCreated = false;

    if (!tooltip) {
        // give the object a chance for delayed loading of the tip
        QMetaObject::invokeMethod(currentWidget, "toolTipAboutToShow");
        tooltip = tooltips.value(currentWidget);
        //kDebug() << "attempt to make one ... we gots" << tooltip;
        if (tooltip) {
            justCreated = true;
        } else {
            currentWidget = 0;
            return;
        }
    }

    tooltip->hide();
    //kDebug() << "about to show" << justCreated;
    tooltip->prepareShowing(!justCreated);
    tooltip->move(ToolTipManager::self()->m_corona->popupPosition(currentWidget, tooltip->size()));
    isShown = true;  //ToolTip is visible
    tooltip->show();

    delayedHide = tooltip->autohide();
    if (delayedHide) {
        //kDebug() << "starting authoide";
        hideTimer->start(3000);
    }
}

bool ToolTipManager::eventFilter(QObject *watched, QEvent *event)
{
    QGraphicsWidget * widget = dynamic_cast<QGraphicsWidget *>(watched);
    if (d->state != Activated || !widget) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::GraphicsSceneHoverMove:
            // If the tooltip isn't visible, run through showing the tooltip again
            // so that it only becomes visible after a stationary hover
            if (Plasma::ToolTipManager::self()->isVisible(widget)) {
                break;
            }

            // Don't restart the show timer on a mouse move event if there hasn't
            // been an enter event or the current widget has been cleared by a click
            // or wheel event.
            if (!d->currentWidget) {
                break;
            }

        case QEvent::GraphicsSceneHoverEnter:
        {
            // Check that there is a tooltip to show
            if (!d->tooltips.contains(widget)) {
                break;
            }

            // If the mouse is in the widget's area at the time that it is being
            // created the widget can receive a hover event before it is fully
            // initialized, in which case view() will return 0.
            QGraphicsView *parentView = viewFor(widget);
            if (parentView) {
                show(widget);
            }

            break;
        }

        case QEvent::GraphicsSceneHoverLeave:
            d->doDelayedHide();
            break;

        case QEvent::GraphicsSceneMousePress:
        case QEvent::GraphicsSceneWheel:
            hide(widget);

        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

} // Plasma namespace

#include "tooltipmanager.moc"

