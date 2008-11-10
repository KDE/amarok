/*
 * Copyright 2008 by Alessandro Diaferia <alediaferia@gmail.com>
 * Copyright 2007 by Alexis Ménard <darktears31@gmail.com>
 * Copyright 2007 Sebastian Kuegler <sebas@kde.org>
 * Copyright 2006 Aaron Seigo <aseigo@kde.org>
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

#include "dialog.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QResizeEvent>
#include <QMouseEvent>
#ifdef Q_WS_X11
#include <QX11Info>
#endif
#include <QBitmap>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGraphicsSceneEvent>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsWidget>

#include <KDebug>
#include <NETRootInfo>

#include "plasma/applet.h"
#include "plasma/extender.h"
#include "plasma/private/extender_p.h"
#include "plasma/framesvg.h"
#include "plasma/theme.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

const int resizeAreaMargin = 20;

namespace Plasma
{

class DialogPrivate
{
public:
    DialogPrivate(Dialog *dialog)
            : q(dialog),
              background(0),
              view(0),
              widget(0),
              resizeCorners(Dialog::NoCorner),
              resizeStartCorner(Dialog::NoCorner)
    {
    }

    ~DialogPrivate()
    {
    }

    void themeUpdated();
    void adjustView();

    Plasma::Dialog *q;
    /**
     * Holds the background SVG, to be re-rendered when the cache is invalidated,
     * for example by resizing the dialogue.
     */
    Plasma::FrameSvg *background;
    QGraphicsView *view;
    QGraphicsWidget *widget;
    Dialog::ResizeCorners resizeCorners;
    QMap<Dialog::ResizeCorner, QRect> resizeAreas;
    Dialog::ResizeCorner resizeStartCorner;
};

void DialogPrivate::themeUpdated()
{
    const int topHeight = background->marginSize(Plasma::TopMargin);
    const int leftWidth = background->marginSize(Plasma::LeftMargin);
    const int rightWidth = background->marginSize(Plasma::RightMargin);
    const int bottomHeight = background->marginSize(Plasma::BottomMargin);

    //TODO: correct handling of the situation when having vertical panels.
    Extender *extender = qobject_cast<Extender*>(widget);
    if (extender) {
        switch (extender->d->applet->location()) {
        case BottomEdge:
            background->setEnabledBorders(FrameSvg::LeftBorder | FrameSvg::TopBorder
                                                               | FrameSvg::RightBorder);
            q->setContentsMargins(0, topHeight, 0, 0);
            break;
        case TopEdge:
            background->setEnabledBorders(FrameSvg::LeftBorder | FrameSvg::BottomBorder
                                                               | FrameSvg::RightBorder);
            q->setContentsMargins(0, 0, 0, bottomHeight);
            break;
        case LeftEdge:
            background->setEnabledBorders(FrameSvg::TopBorder | FrameSvg::BottomBorder
                                                              | FrameSvg::RightBorder);
            q->setContentsMargins(0, topHeight, 0, bottomHeight);
            break;
        case RightEdge:
            background->setEnabledBorders(FrameSvg::TopBorder | FrameSvg::BottomBorder
                                                              | FrameSvg::LeftBorder);
            q->setContentsMargins(0, topHeight, 0, bottomHeight);
            break;
        default:
            background->setEnabledBorders(FrameSvg::AllBorders);
            q->setContentsMargins(leftWidth, topHeight, rightWidth, bottomHeight);
        }
    } else {
        q->setContentsMargins(leftWidth, topHeight, rightWidth, bottomHeight);
    }
    q->update();
}

void DialogPrivate::adjustView()
{
    if (view && widget) {
        QSize prevSize = q->size();
        /*
        kDebug() << "Widget size:" << widget->size()
                 << "| Widget size hint:" << widget->effectiveSizeHint(Qt::PreferredSize)
                 << "| Widget minsize hint:" << widget->minimumSize()
                 << "| Widget maxsize hint:" << widget->maximumSize()
                 << "| Widget bounding rect:" << widget->boundingRect();
        */
        //set the sizehints correctly:
        int left, top, right, bottom;
        q->getContentsMargins(&left, &top, &right, &bottom);

        q->setMinimumSize(qMin(int(widget->minimumSize().width()) + left + right, QWIDGETSIZE_MAX),
                          qMin(int(widget->minimumSize().height()) + top + bottom, QWIDGETSIZE_MAX));
        q->setMaximumSize(qMin(int(widget->maximumSize().width()) + left + right, QWIDGETSIZE_MAX),
                          qMin(int(widget->maximumSize().height()) + top + bottom, QWIDGETSIZE_MAX));
        q->resize(qMin(int(view->size().width()) + left + right, QWIDGETSIZE_MAX),
                          qMin(int(view->size().height()) + top + bottom, QWIDGETSIZE_MAX));
        q->updateGeometry();

        //reposition and resize the view.
        view->setSceneRect(widget->sceneBoundingRect());
        view->resize(view->mapFromScene(view->sceneRect()).boundingRect().size());
        view->centerOn(widget);

        if (q->size() != prevSize) {
            //the size of the dialog has changed, emit the signal:
            emit q->dialogResized();
        }
    }
}

Dialog::Dialog(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f),
      d(new DialogPrivate(this))
{
    setWindowFlags(Qt::FramelessWindowHint);
    d->background = new FrameSvg(this);
    d->background->setImagePath("dialogs/background");
    d->background->setEnabledBorders(FrameSvg::AllBorders);
    d->background->resizeFrame(size());

    connect(d->background, SIGNAL(repaintNeeded()), this, SLOT(update()));

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeUpdated()));
    d->themeUpdated();

    setMouseTracking(true);
}

Dialog::~Dialog()
{
    delete d;
}

void Dialog::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setClipRect(e->rect());
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), Qt::transparent);
    d->background->paintFrame(&p);

    //we set the resize handlers
    d->resizeAreas.clear();
    if (d->resizeCorners & Dialog::NorthEast) {
        d->resizeAreas[Dialog::NorthEast] = QRect(rect().right() - resizeAreaMargin, 0,
                                             resizeAreaMargin, resizeAreaMargin);
    }

    if (d->resizeCorners & Dialog::NorthWest) {
        d->resizeAreas[Dialog::NorthWest] = QRect(0, 0, resizeAreaMargin, resizeAreaMargin);
    }

    if (d->resizeCorners & Dialog::SouthEast) {
        d->resizeAreas[Dialog::SouthEast] = QRect(rect().right() - resizeAreaMargin,
                                            rect().bottom() - resizeAreaMargin,
                                            resizeAreaMargin, resizeAreaMargin);
    }

    if (d->resizeCorners & Dialog::SouthWest) {
        d->resizeAreas[Dialog::SouthWest] = QRect(0, rect().bottom() - resizeAreaMargin,
                                            resizeAreaMargin, resizeAreaMargin);
    }
}

void Dialog::mouseMoveEvent(QMouseEvent *event)
{
    if (d->resizeAreas[Dialog::NorthEast].contains(event->pos()) && d->resizeCorners & Dialog::NorthEast) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (d->resizeAreas[Dialog::NorthWest].contains(event->pos()) && d->resizeCorners & Dialog::NorthWest) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (d->resizeAreas[Dialog::SouthEast].contains(event->pos()) && d->resizeCorners & Dialog::SouthEast) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (d->resizeAreas[Dialog::SouthWest].contains(event->pos()) && d->resizeCorners & Dialog::SouthWest) {
        setCursor(Qt::SizeBDiagCursor);
    } else {
        unsetCursor();
    }

    // here we take care of resize..
    if (d->resizeStartCorner != Dialog::NoCorner) {
        int newWidth;
        int newHeight;
        QPoint position;

        switch(d->resizeStartCorner) {
            case Dialog::NorthEast:
                newWidth = event->x();
                newHeight = height() - event->y();
                position = QPoint(x(), y() + height() - newHeight);
                break;
            case Dialog::NorthWest:
                newWidth = width() - event->x();
                newHeight = height() - event->y();
                position = QPoint(x() + width() - newWidth, y() + height() - newHeight);
                break;
            case Dialog::SouthWest:
                newWidth = width() - event->x();
                newHeight = event->y();
                position = QPoint(x() + width() - newWidth, y());
                break;
            case Dialog::SouthEast:
                newWidth = event->x();
                newHeight = event->y();
                position = QPoint(x(), y());
                break;
             default:
                newWidth = width();
                newHeight = height();
                position = QPoint(x(), y());
                break;
        }

        // let's check for limitations
        if (newWidth < minimumWidth() || newWidth > maximumWidth()) {
            newWidth = width();
            position.setX(x());
        }

        if (newHeight < minimumHeight() || newHeight > maximumHeight()) {
            newHeight = height();
            position.setY(y());
        }

        setGeometry(QRect(position, QSize(newWidth, newHeight)));
    }

    QWidget::mouseMoveEvent(event);
}

void Dialog::mousePressEvent(QMouseEvent *event)
{
    if (d->resizeAreas[Dialog::NorthEast].contains(event->pos()) && d->resizeCorners & Dialog::NorthEast) {
        d->resizeStartCorner = Dialog::NorthEast;

    } else if (d->resizeAreas[Dialog::NorthWest].contains(event->pos()) && d->resizeCorners & Dialog::NorthWest) {
        d->resizeStartCorner = Dialog::NorthWest;

    } else if (d->resizeAreas[Dialog::SouthEast].contains(event->pos()) && d->resizeCorners & Dialog::SouthEast) {
        d->resizeStartCorner = Dialog::SouthEast;

    } else if (d->resizeAreas[Dialog::SouthWest].contains(event->pos()) && d->resizeCorners & Dialog::SouthWest) {
        d->resizeStartCorner = Dialog::SouthWest;

    } else {
        d->resizeStartCorner = Dialog::NoCorner;
    }

    QWidget::mousePressEvent(event);
}

void Dialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->resizeStartCorner != Dialog::NoCorner) {
        d->resizeStartCorner = Dialog::NoCorner;
        emit dialogResized();
    }

    QWidget::mouseReleaseEvent(event);
}

void Dialog::resizeEvent(QResizeEvent *e)
{
    d->background->resizeFrame(e->size());

    setMask(d->background->mask());

    if (d->resizeStartCorner != Dialog::NoCorner && d->view && d->widget) {
        d->widget->setPreferredSize(d->view->size());

        QGraphicsLayoutItem *layout = d->widget->parentLayoutItem();
        QGraphicsWidget *parentWidget = d->widget->parentWidget();

        if (layout && parentWidget) {
            layout->updateGeometry();
            parentWidget->resize(layout->preferredSize());
        }

        d->view->setSceneRect(d->widget->mapToScene(d->widget->boundingRect()).boundingRect());
        d->view->centerOn(d->widget);
    }
}

void Dialog::setGraphicsWidget(QGraphicsWidget *widget)
{
    if (d->widget) {
        d->widget->removeEventFilter(this);
    }

    d->widget = widget;

    if (widget) {
        if (!layout()) {
            QVBoxLayout *lay = new QVBoxLayout(this);
            lay->setMargin(0);
            lay->setSpacing(0);
        }

        d->themeUpdated();

        if (!d->view) {
            d->view = new QGraphicsView(this);
            d->view->setFrameShape(QFrame::NoFrame);
            d->view->viewport()->setAutoFillBackground(false);
            layout()->addWidget(d->view);
        }

        d->view->setScene(widget->scene());
        d->adjustView();

        adjustSize();

        widget->installEventFilter(this);
    } else {
        delete d->view;
        d->view = 0;
    }
}

QGraphicsWidget *Dialog::graphicsWidget()
{
    return d->widget;
}

bool Dialog::eventFilter(QObject *watched, QEvent *event)
{
    if (d->resizeStartCorner == Dialog::NoCorner && watched == d->widget &&
        (event->type() == QEvent::GraphicsSceneResize || event->type() == QEvent::GraphicsSceneMove)) {
        d->adjustView();
    }

    return QWidget::eventFilter(watched, event);
}

void Dialog::hideEvent(QHideEvent * event)
{
    Q_UNUSED(event);
    emit dialogVisible(false);
}

void Dialog::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
    emit dialogVisible(true);
}

void Dialog::setResizeHandleCorners(ResizeCorners corners)
{
    d->resizeCorners = corners;
    update();
}

Dialog::ResizeCorners Dialog::resizeCorners() const
{
    return d->resizeCorners;
}

bool Dialog::inControlArea(const QPoint &point)
{
    foreach (const QRect &r, d->resizeAreas) {
        if (r.contains(point)) {
            return true;
        }
    }
    return false;
}

}
#include "dialog.moc"
