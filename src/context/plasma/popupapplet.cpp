/*
 * Copyright 2008 by Montel Laurent <montel@kde.org>
 * Copyright 2008 by Marco Martin <notmart@gmail.com>
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

#include "popupapplet.h"
#include "private/popupapplet_p.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QApplication>

#include <KIcon>
#include <KIconLoader>
#include <KWindowSystem>
#include <KGlobalSettings>

#include "plasma/private/applet_p.h"
#include "plasma/dialog.h"
#include "plasma/corona.h"
#include "plasma/containment.h"
#include "plasma/extender.h"
#include "plasma/widgets/iconwidget.h"

namespace Plasma
{

PopupApplet::PopupApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args),
      d(new PopupAppletPrivate(this))
{
    int iconSize = IconSize(KIconLoader::Desktop);
    resize(iconSize, iconSize);
    connect(this, SIGNAL(activate()), this, SLOT(togglePopup()));
}

PopupApplet::~PopupApplet()
{
    delete widget();
    delete d;
}

void PopupApplet::setPopupIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        if (d->icon) {
            delete d->icon;
            d->icon = 0;
            setLayout(0);
        }
        return;
    }

    if (!d->icon) {
        d->icon = new Plasma::IconWidget(icon, QString(), this);
        connect(d->icon, SIGNAL(clicked()), this, SLOT(togglePopup()));

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setOrientation(Qt::Horizontal);

        if (formFactor() == Plasma::Vertical || formFactor() == Plasma::Horizontal ) {
            d->savedAspectRatio = aspectRatioMode();
            setAspectRatioMode(Plasma::ConstrainedSquare);
        }

        setLayout(layout);
    } else {
        d->icon->setIcon(icon);
    }
}

void PopupApplet::setPopupIcon(const QString &iconName)
{
    setPopupIcon(KIcon(iconName));
}

QIcon PopupApplet::popupIcon() const
{
    return d->icon ? d->icon->icon() : QIcon();
}

QWidget *PopupApplet::widget()
{
    return 0;
}

QGraphicsWidget *PopupApplet::graphicsWidget()
{
    return static_cast<Applet*>(this)->d->extender;
}

void PopupAppletPrivate::popupConstraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::StartupCompletedConstraint) {
        startupComplete = true;
    }

    if (!startupComplete) {
        return;
    }

    Plasma::FormFactor f = q->formFactor();
    if (constraints & Plasma::FormFactorConstraint ||
        (constraints & Plasma::SizeConstraint &&
         (f == Plasma::Vertical || f == Plasma::Horizontal))) {
        QGraphicsLinearLayout *lay = dynamic_cast<QGraphicsLinearLayout *>(q->layout());

        if (icon && !icon->icon().isNull() && lay) {
            lay->removeAt(0);
        }

        QSizeF minimum;
        QSizeF containmentSize;

        QGraphicsWidget *gWidget = q->graphicsWidget();
        kDebug() << "graphics widget is" << (QObject*)gWidget;
        QWidget *qWidget = q->widget();

        if (gWidget) {
            minimum = gWidget->minimumSize();
            // our layout may have been replaced on us in the call to graphicsWidget!
            lay = dynamic_cast<QGraphicsLinearLayout *>(q->layout());
        } else if (qWidget) {
            minimum = qWidget->minimumSizeHint();
        }

        if (q->containment()) {
            containmentSize = q->containment()->size();
        }

        if (icon && !icon->icon().isNull() && ((f != Plasma::Vertical && f != Plasma::Horizontal) ||
            ((f == Plasma::Vertical && containmentSize.width() >= minimum.width()) ||
             (f == Plasma::Horizontal && containmentSize.height() >= minimum.height())))) {
            // we only switch to expanded if we aren't horiz/vert constrained and
            // this applet has an icon.
            // otherwise, we leave it up to the applet itself to figure it out
            if (icon) {
                icon->hide();
            }

            if (savedAspectRatio != Plasma::InvalidAspectRatioMode) {
                q->setAspectRatioMode(savedAspectRatio);
            }

            if (dialog) {
                if (dialog->layout() && qWidget) {
                    //we don't want to delete Widget inside the dialog layout
                    dialog->layout()->removeWidget(qWidget);
                }

                if (qWidget) {
                    qWidget->setParent(0);
                }

                delete dialog;
                dialog = 0;
            }

            if (!lay && !q->layout()) {
                lay = new QGraphicsLinearLayout();
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(0);
                lay->setOrientation(Qt::Horizontal);
                q->setLayout(lay);
            }

            if (gWidget) {
                Extender *extender = qobject_cast<Extender*>(gWidget);
                if (extender) {
                    extender->setExtenderAppearance(Extender::NoBorders);
                }

                lay->addItem(gWidget);
            } else if (qWidget) {
                if (!proxy) {
                    proxy = new QGraphicsProxyWidget(q);
                    proxy->setWidget(qWidget);
                    proxy->show();
                }

                if (lay) {
                    lay->addItem(proxy);
                }
            }

            qreal left, top, right, bottom;
            q->getContentsMargins(&left, &top, &right, &bottom);
            q->setMinimumSize(minimum + QSizeF(left+right, top+bottom));
        } else {
            //save the aspect ratio mode in case we drag'n drop in the Desktop later
            savedAspectRatio = q->aspectRatioMode();
            q->setAspectRatioMode(Plasma::ConstrainedSquare);

            if (icon) {
                icon->show();
            }

            if (proxy) {
                proxy->setWidget(0); // prevent it from deleting our widget!
                delete proxy;
                proxy = 0;
            }

            if (!dialog) {
                dialog = new Plasma::Dialog();

                //no longer use Qt::Popup since that seems to cause a lot of problem when you drag
                //stuff out of your Dialog (extenders). Monitor WindowDeactivate events so we can
                //emulate the same kind of behavior as Qt::Popup (close when you click somewhere
                //else.
                dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
                KWindowSystem::setState(dialog->winId(), NET::SkipTaskbar | NET::SkipPager);
                dialog->installEventFilter(q);

                QObject::connect(dialog, SIGNAL(dialogResized()),
                                 q, SLOT(dialogSizeChanged()));
                QObject::connect(dialog, SIGNAL(dialogVisible(bool)),
                                 q, SLOT(dialogStatusChanged(bool)));
                q->setMinimumSize(QSize(0, 0));
                if (gWidget) {
                    Corona *corona = qobject_cast<Corona *>(gWidget->scene());

                    Extender *extender = qobject_cast<Extender*>(gWidget);
                    if (extender) {
                        if (q->formFactor() == MediaCenter || q->formFactor() == Planar) {
                            extender->setExtenderAppearance(Extender::NoBorders);
                        } else if (q->location() == TopEdge) {
                            extender->setExtenderAppearance(Extender::TopDownStacked);
                        } else {
                            extender->setExtenderAppearance(Extender::BottomUpStacked);
                        }
                    }

                    //could that cast ever fail??
                    if (corona) {
                        corona->addOffscreenWidget(gWidget);
                        gWidget->resize(gWidget->preferredSize());
                        gWidget->setMinimumSize(gWidget->preferredSize());
                        dialog->setGraphicsWidget(gWidget);
                    }
                } else if (qWidget) {
                    QVBoxLayout *l_layout = new QVBoxLayout(dialog);
                    l_layout->setSpacing(0);
                    l_layout->setMargin(0);
                    l_layout->addWidget(qWidget);
                }
            }

            dialog->adjustSize();

            if (icon && lay) {
                lay->addItem(icon);
            }

            q->setMinimumSize(0,0);
        }
    }
}

void PopupApplet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!d->icon && !d->popupLostFocus && event->buttons() == Qt::LeftButton) {
        d->clicked = scenePos().toPoint();
        event->setAccepted(true);
        return;
    } else {
        d->popupLostFocus = false;
        Applet::mousePressEvent(event);
    }
}

void PopupApplet::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!d->icon &&
        (d->clicked - scenePos().toPoint()).manhattanLength() < KGlobalSettings::dndEventDelay()) {
        d->togglePopup();
    } else {
        Applet::mouseReleaseEvent(event);
    }
}

bool PopupApplet::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d->dialog && (event->type() == QEvent::WindowDeactivate)) {
        d->popupLostFocus = true;
        hidePopup();
        QTimer::singleShot(100, this, SLOT(clearPopupLostFocus()));
    }

    /**
    if (layout() && watched == graphicsWidget() && (event->type() == QEvent::GraphicsSceneResize)) {
        //sizes are recalculated in the constraintsevent so let's just call that.
        d->popupConstraintsEvent(Plasma::FormFactorConstraint);

        //resize vertically if necesarry.
        if (formFactor() == Plasma::MediaCenter || formFactor() == Plasma::Planar) {
            resize(QSizeF(size().width(), minimumHeight()));
        }
    }
    */

    return Applet::eventFilter(watched, event);
}

void PopupApplet::showPopup(uint popupDuration)
{
    if (d->dialog && (formFactor() == Horizontal || formFactor() == Vertical)) {
        d->updateDialogPosition();
        d->dialog->show();
        KWindowSystem::setState(d->dialog->winId(), NET::SkipTaskbar | NET::SkipPager);

        if (d->timer) {
            d->timer->stop();
        }

        if (popupDuration > 0) {
            if (!d->timer) {
                d->timer = new QTimer(this);
                connect(d->timer, SIGNAL(timeout()), this, SLOT(hideTimedPopup()));
            }

            d->timer->start(popupDuration);
        }
    }
}

void PopupApplet::hidePopup()
{
    if (d->dialog && (formFactor() == Horizontal || formFactor() == Vertical)) {
        d->dialog->hide();
    }
}

Plasma::PopupPlacement PopupApplet::popupPlacement() const
{
    return d->popupPlacement;
}

void PopupApplet::popupEvent(bool)
{

}

PopupAppletPrivate::PopupAppletPrivate(PopupApplet *applet)
        : q(applet),
          icon(0),
          dialog(0),
          proxy(0),
          popupPlacement(Plasma::FloatingPopup),
          savedAspectRatio(Plasma::InvalidAspectRatioMode),
          timer(0),
          startupComplete(false),
          popupLostFocus(false)
{
}

PopupAppletPrivate::~PopupAppletPrivate()
{
    if (proxy) {
        proxy->setWidget(0);
    }

    delete dialog;
    delete icon;
}

void PopupAppletPrivate::togglePopup()
{
    if (dialog) {
        if (timer) {
            timer->stop();
        }

        if (dialog->isVisible()) {
            dialog->hide();
        } else {
            updateDialogPosition();
            dialog->show();
            KWindowSystem::setState(dialog->winId(), NET::SkipTaskbar | NET::SkipPager);
        }

        dialog->clearFocus();
    }
}

void PopupAppletPrivate::hideTimedPopup()
{
    timer->stop();
    q->hidePopup();
}

void PopupAppletPrivate::clearPopupLostFocus()
{
    popupLostFocus = false;
}

void PopupAppletPrivate::dialogSizeChanged()
{
    //Reposition the dialog
    if (dialog) {
        dialog->updateGeometry();
        dialog->move(q->popupPosition(dialog->size()));

        KConfigGroup sizeGroup = q->config();
        sizeGroup = KConfigGroup(&sizeGroup, "PopupApplet");
        sizeGroup.writeEntry("DialogHeight", dialog->height());
        sizeGroup.writeEntry("DialogWidth", dialog->width());

        emit q->configNeedsSaving();
    }
}

void PopupAppletPrivate::dialogStatusChanged(bool status)
{
    q->popupEvent(status);
}

void PopupAppletPrivate::updateDialogPosition()
{
    QGraphicsView *view = q->view();

    if (!view) {
        return;
    }

    KConfigGroup sizeGroup = q->config();
    sizeGroup = KConfigGroup(&sizeGroup, "PopupApplet");
    
    Q_ASSERT(q->containment());
    Q_ASSERT(q->containment()->corona());
    const int width = qMin(sizeGroup.readEntry("DialogWidth", 0),
                           q->containment()->corona()->screenGeometry(-1).width() - 50);
    const int height = qMin(sizeGroup.readEntry("DialogHeight", 0),
                            q->containment()->corona()->screenGeometry(-1).height() - 50);

    QSize saved(width, height);

    if (saved.isNull()) {
        dialog->adjustSize();
    } else {
        saved = saved.expandedTo(dialog->minimumSizeHint());
        dialog->resize(saved);
    }

    QSize s = dialog->size();
    QPoint pos = view->mapFromScene(q->scenePos());
    pos = view->mapToGlobal(pos);

    switch (q->location()) {
    case BottomEdge:
        pos = QPoint(pos.x(), pos.y() - s.height());
        popupPlacement = Plasma::TopPosedLeftAlignedPopup;
        dialog->setResizeHandleCorners(Dialog::NorthEast);

        break;
    case TopEdge:
        pos = QPoint(pos.x(), pos.y() + (int)q->boundingRect().size().height());
        popupPlacement = Plasma::BottomPosedLeftAlignedPopup;
        dialog->setResizeHandleCorners(Dialog::SouthEast);

        break;
    case LeftEdge:
        pos = QPoint(pos.x() + (int)q->boundingRect().size().width(), pos.y());
        popupPlacement = Plasma::RightPosedTopAlignedPopup;
        dialog->setResizeHandleCorners(Dialog::SouthEast);

        break;

    case RightEdge:
        pos = QPoint(pos.x() - s.width(), pos.y());
        popupPlacement = Plasma::LeftPosedTopAlignedPopup;
        dialog->setResizeHandleCorners(Dialog::SouthWest);

        break;
    default:
        if (pos.y() - s.height() > 0) {
            pos = QPoint(pos.x(), pos.y() - s.height());
        } else {
            pos = QPoint(pos.x(), pos.y() + (int)q->boundingRect().size().height());
        }

        dialog->setResizeHandleCorners(Dialog::NorthEast);
    }
    //are we out of screen?

    QRect screenRect =
        q->containment()->corona()->screenGeometry(q->containment() ? q->containment()->screen() : -1);
    //kDebug() << "==> rect for"
    //         << (containment() ? containment()->screen() : -1)
    //         << "is" << screenRect;

    if (pos.rx() + s.width() > screenRect.right()) {
        pos.rx() += (int)q->boundingRect().size().width() - s.width();

        if (q->location() == BottomEdge) {
            popupPlacement = Plasma::TopPosedRightAlignedPopup;
            dialog->setResizeHandleCorners(Dialog::NorthWest);
        } else if (q->location() == TopEdge) {
            popupPlacement = Plasma::BottomPosedRightAlignedPopup;
            dialog->setResizeHandleCorners(Dialog::SouthWest);
        }
    }

    if (pos.ry() + s.height() > screenRect.bottom()) {
        pos.ry() += (int)q->boundingRect().size().height() - s.height();

        if (q->location() == LeftEdge) {
            popupPlacement = Plasma::RightPosedBottomAlignedPopup;
            dialog->setResizeHandleCorners(Dialog::NorthEast);
        } else if (q->location() == RightEdge) {
            popupPlacement = Plasma::LeftPosedBottomAlignedPopup;
            dialog->setResizeHandleCorners(Dialog::NorthWest);
        }
    }

    pos.rx() = qMax(0, pos.rx());

    dialog->move(pos);
}
} // Plasma namespace

#include "popupapplet.moc"

