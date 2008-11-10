/*
 *   Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 Marco Martin <notmart@gmail.com>
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

#include "framesvg.h"

#include <QPainter>
#include <QSize>
#include <QBitmap>

#include <KDebug>

#include <plasma/theme.h>

namespace Plasma
{

class FrameData
{
public:
    FrameData()
      : enabledBorders(FrameSvg::AllBorders),
        frameSize(-1,-1)
    {
    }

    FrameData(const FrameData &other)
      : enabledBorders(other.enabledBorders),
        frameSize(other.frameSize)
    {
    }

    ~FrameData()
    {
    }

    FrameSvg::EnabledBorders enabledBorders;
    QPixmap cachedBackground;
    QBitmap cachedMask;
    QSizeF frameSize;

    //measures
    int topHeight;
    int leftWidth;
    int rightWidth;
    int bottomHeight;

    //margins, are equal to the measures by default
    int topMargin;
    int leftMargin;
    int rightMargin;
    int bottomMargin;

    //size of the svg where the size of the "center"
    //element is contentWidth x contentHeight
    bool noBorderPadding : 1;
    bool stretchBorders : 1;
    bool tileCenter : 1;
};

class FrameSvgPrivate
{
public:
    FrameSvgPrivate(FrameSvg *psvg)
      : q(psvg),
        cacheAll(false)
    {
    }

    ~FrameSvgPrivate()
    {
        qDeleteAll(frames);
        frames.clear();
    }

    void generateBackground(FrameData *frame);
    void updateSizes();
    void updateNeeded();
    void updateAndSignalSizes();

    Location location;
    QString prefix;

    FrameSvg *q;

    bool cacheAll : 1;

    QHash<QString, FrameData*> frames;
};

FrameSvg::FrameSvg(QObject *parent)
    : Svg(parent),
      d(new FrameSvgPrivate(this))
{
    connect(this, SIGNAL(repaintNeeded()), this, SLOT(updateNeeded()));
    d->frames.insert(QString(), new FrameData());
}

FrameSvg::~FrameSvg()
{
    delete d;
}

void FrameSvg::setImagePath(const QString &path)
{
    if (path == imagePath()) {
        return;
    }

    Svg::setImagePath(path);
    setContainsMultipleImages(true);

    clearCache();
    d->updateAndSignalSizes();
}

void FrameSvg::setEnabledBorders(const EnabledBorders borders)
{
    if (borders == d->frames[d->prefix]->enabledBorders) {
        return;
    }

    d->frames[d->prefix]->enabledBorders = borders;
    d->updateAndSignalSizes();
}

FrameSvg::EnabledBorders FrameSvg::enabledBorders() const
{
    QHash<QString, FrameData*>::const_iterator it = d->frames.constFind(d->prefix);

    if (it != d->frames.constEnd()) {
        return it.value()->enabledBorders;
    } else {
        return NoBorder;
    }
}

void FrameSvg::setElementPrefix(Plasma::Location location)
{
    switch (location) {
        case TopEdge:
            setElementPrefix("north");
            break;
        case BottomEdge:
            setElementPrefix("south");
            break;
        case LeftEdge:
            setElementPrefix("west");
            break;
        case RightEdge:
            setElementPrefix("east");
            break;
        default:
            setElementPrefix(QString());
            break;
    }
    d->location = location;
}

void FrameSvg::setElementPrefix(const QString & prefix)
{
    const QString oldPrefix(d->prefix);

    if (!hasElement(prefix + "-center")) {
        d->prefix.clear();
    } else {
        d->prefix = prefix;
        if (!d->prefix.isEmpty()) {
            d->prefix += '-';
        }

    }

    if (oldPrefix == d->prefix && d->frames[oldPrefix]) {
        return;
    }

    if (!d->frames.contains(d->prefix)) {
        d->frames.insert(d->prefix, new FrameData(*(d->frames[oldPrefix])));
        d->updateSizes();
    }

    if (!d->cacheAll) {
        delete d->frames[oldPrefix];
        d->frames.remove(oldPrefix);
    }

    d->location = Floating;
}

bool FrameSvg::hasElementPrefix(const QString & prefix) const
{
    //for now it simply checks if a center element exists,
    //because it could make sense for certain themes to not have all the elements
    if (prefix.isEmpty()) {
        return hasElement("center");
    } else {
        return hasElement(prefix + "-center");
    }
}

bool FrameSvg::hasElementPrefix(Plasma::Location location) const
{
    switch (location) {
        case TopEdge:
            return hasElementPrefix("north");
            break;
        case BottomEdge:
            return hasElementPrefix("south");
            break;
        case LeftEdge:
            return hasElementPrefix("west");
            break;
        case RightEdge:
            return hasElementPrefix("east");
            break;
        default:
            return hasElementPrefix(QString());
            break;
    }
}

QString FrameSvg::prefix()
{
    if (d->prefix.isEmpty()) {
        return d->prefix;
    }

    return d->prefix.left(d->prefix.size() - 1);
}

void FrameSvg::resizeFrame(const QSizeF &size)
{
    if (size.isEmpty()) {
        kWarning() << "Invalid size" << size;
        return;
    }

    if (size == d->frames[d->prefix]->frameSize) {
        return;
    }

    d->updateSizes();
    d->frames[d->prefix]->frameSize = size;
}

QSizeF FrameSvg::frameSize() const
{
    QHash<QString, FrameData*>::const_iterator it = d->frames.constFind(d->prefix);

    if (it != d->frames.constEnd()) {
        return it.value()->frameSize;
    } else {
        return QSize(-1, -1);
    }
}

qreal FrameSvg::marginSize(const Plasma::MarginEdge edge) const
{
    if (d->frames[d->prefix]->noBorderPadding) {
        return .0;
    }

    switch (edge) {
    case Plasma::TopMargin:
        return d->frames[d->prefix]->topMargin;
    break;

    case Plasma::LeftMargin:
        return d->frames[d->prefix]->leftMargin;
    break;

    case Plasma::RightMargin:
        return d->frames[d->prefix]->rightMargin;
    break;

    //Plasma::BottomMargin
    default:
        return d->frames[d->prefix]->bottomMargin;
    break;
    }
}

void FrameSvg::getMargins(qreal &left, qreal &top, qreal &right, qreal &bottom) const
{
    FrameData *frame = d->frames[d->prefix];

    if (!frame || frame->noBorderPadding) {
        left = top = right = bottom = 0;
        return;
    }

    top = frame->topMargin;
    left = frame->leftMargin;
    right = frame->rightMargin;
    bottom = frame->bottomMargin;
}

QRectF FrameSvg::contentsRect() const
{
    QSizeF size(frameSize());

    if (size.isValid()) {
        QRectF rect(QPointF(0, 0), size);
        FrameData *frame = d->frames[d->prefix];

        return rect.adjusted(frame->leftMargin, frame->topMargin,
                             -frame->rightMargin, -frame->bottomMargin);
    } else {
        return QRectF();
    }
}

QBitmap FrameSvg::mask() const
{
    FrameData *frame = d->frames[d->prefix];

    if (!frame->cachedMask) {
        if (frame->cachedBackground.isNull()) {
            d->generateBackground(frame);
            if (frame->cachedBackground.isNull()) {
                return QBitmap();
            }
        }
        frame->cachedMask = QBitmap(frame->cachedBackground.alphaChannel().createMaskFromColor(Qt::black));
    }
    return frame->cachedMask;
}

void FrameSvg::setCacheAllRenderedFrames(bool cache)
{
    if (d->cacheAll && !cache) {
        clearCache();
    }

    d->cacheAll = cache;
}

bool FrameSvg::cacheAllRenderedFrames() const
{
    return d->cacheAll;
}

void FrameSvg::clearCache()
{
    FrameData *frame = d->frames[d->prefix];

    // delete all the frames that aren't this one
    QMutableHashIterator<QString, FrameData*> it(d->frames);
    while (it.hasNext()) {
        FrameData *p = it.next().value();
        if (frame != p) {
            delete p;
            it.remove();
        }
    }
}

QPixmap FrameSvg::framePixmap()
{
    FrameData *frame = d->frames[d->prefix];
    if (frame->cachedBackground.isNull()) {
        d->generateBackground(frame);
        if (frame->cachedBackground.isNull()) {
            return QPixmap();
        }
    }

    return frame->cachedBackground;
}

void FrameSvg::paintFrame(QPainter *painter, const QRectF &target, const QRectF &source)
{
    FrameData *frame = d->frames[d->prefix];
    if (frame->cachedBackground.isNull()) {
        d->generateBackground(frame);
        if (frame->cachedBackground.isNull()) {
            return;
        }
    }

    painter->drawPixmap(target, frame->cachedBackground, source.isValid() ? source : target);
}

void FrameSvg::paintFrame(QPainter *painter, const QPointF &pos)
{
    FrameData *frame = d->frames[d->prefix];
    if (frame->cachedBackground.isNull()) {
        d->generateBackground(frame);
        if (frame->cachedBackground.isNull()) {
            return;
        }
    }

    painter->drawPixmap(pos, frame->cachedBackground);
}

void FrameSvgPrivate::generateBackground(FrameData *frame)
{
    if (!frame->cachedBackground.isNull()) {
        return;
    }

    QString id = QString::fromLatin1("%5_%4_%3_%2_%1_").
                         arg(frame->enabledBorders).arg(frame->frameSize.width()).arg(frame->frameSize.height()).arg(prefix).arg(q->imagePath());
    Theme *theme = Theme::defaultTheme();
    if (theme->findInCache(id, frame->cachedBackground) && !frame->cachedBackground.isNull()) {
        return;
    }

    //kDebug() << "generating background";
    const int topWidth = q->elementSize(prefix + "top").width();
    const int leftHeight = q->elementSize(prefix + "left").height();
    const int topOffset = 0;
    const int leftOffset = 0;


    if (!frame->frameSize.isValid()) {
        kWarning() << "Invalid frame size" << frame->frameSize;
        return;
    }

    const int contentWidth = frame->frameSize.width() - frame->leftWidth  - frame->rightWidth;
    const int contentHeight = frame->frameSize.height() - frame->topHeight  - frame->bottomHeight;
    int contentTop = 0;
    int contentLeft = 0;
    int rightOffset = contentWidth;
    int bottomOffset = contentHeight;

    frame->cachedBackground = QPixmap(frame->leftWidth + contentWidth + frame->rightWidth,
                                      frame->topHeight + contentHeight + frame->bottomHeight);
    frame->cachedBackground.fill(Qt::transparent);
    QPainter p(&frame->cachedBackground);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    //if we must stretch the center or the borders we compute how much we will have to stretch
    //the svg to get the desired element sizes
    QSizeF  scaledContentSize(0, 0);
    if (q->elementSize(prefix + "center").width() > 0 &&
        q->elementSize(prefix + "center").height() > 0 &&
        (!frame->tileCenter || frame->stretchBorders)) {
        scaledContentSize = QSizeF(contentWidth * ((qreal)q->size().width() / (qreal)q->elementSize(prefix + "center").width()),
                                   contentHeight * ((qreal)q->size().height() / (qreal)q->elementSize(prefix + "center").height()));
    }

    //CENTER
    if (frame->tileCenter) {
        if (contentHeight > 0 && contentWidth > 0) {
            int centerTileHeight;
            int centerTileWidth;
            centerTileHeight = q->elementSize(prefix + "center").height();
            centerTileWidth = q->elementSize(prefix + "center").width();
            QPixmap center(centerTileWidth, centerTileHeight);
            center.fill(Qt::transparent);

            {
                QPainter centerPainter(&center);
                centerPainter.setCompositionMode(QPainter::CompositionMode_Source);
                q->paint(&centerPainter, QRect(QPoint(0, 0), q->elementSize(prefix + "center")), prefix + "center");
            }

            p.drawTiledPixmap(QRect(frame->leftWidth, frame->topHeight,
                                    contentWidth, contentHeight), center);
        }
    } else {
        if (contentHeight > 0 && contentWidth > 0) {
            q->paint(&p, QRect(frame->leftWidth, frame->topHeight,
                               contentWidth, contentHeight),
                               prefix + "center");
        }
    }

    // Corners
    if (q->hasElement(prefix + "top") && frame->enabledBorders & FrameSvg::TopBorder) {
        contentTop = frame->topHeight;
        bottomOffset += frame->topHeight;

        if (q->hasElement(prefix + "topleft") && frame->enabledBorders & FrameSvg::LeftBorder) {
            q->paint(&p, QRect(leftOffset, topOffset, frame->leftWidth, frame->topHeight), prefix + "topleft");

            contentLeft = frame->leftWidth;
            rightOffset = contentWidth + frame->leftWidth;
        }

        if (q->hasElement(prefix + "topright") && frame->enabledBorders & FrameSvg::RightBorder) {
            q->paint(&p, QRect(rightOffset, topOffset, frame->rightWidth, frame->topHeight), prefix + "topright");
        }
    }

    if (q->hasElement(prefix + "bottom") && frame->enabledBorders & FrameSvg::BottomBorder) {
        if (q->hasElement(prefix + "bottomleft") && frame->enabledBorders & FrameSvg::LeftBorder) {
            q->paint(&p, QRect(leftOffset, bottomOffset, frame->leftWidth, frame->bottomHeight), prefix + "bottomleft");

            contentLeft = frame->leftWidth;
            rightOffset = contentWidth + frame->leftWidth;
        }

        if (q->hasElement(prefix + "bottomright") && frame->enabledBorders & FrameSvg::RightBorder) {
            q->paint(&p, QRect(rightOffset, bottomOffset, frame->rightWidth, frame->bottomHeight), prefix + "bottomright");
        }
    }

    // Sides
    if (frame->stretchBorders) {
        if (frame->enabledBorders & FrameSvg::LeftBorder || frame->enabledBorders & FrameSvg::RightBorder) {

            if (q->hasElement(prefix + "left") &&
                frame->enabledBorders & FrameSvg::LeftBorder) {
                q->paint(&p, QRect(leftOffset, contentTop, frame->leftWidth, contentHeight), prefix + "left");
            }

            if (q->hasElement(prefix + "right") &&
                frame->enabledBorders & FrameSvg::RightBorder) {
                q->paint(&p, QRect(rightOffset, contentTop, frame->rightWidth, contentHeight), prefix + "right");
            }
        }

        if (frame->enabledBorders & FrameSvg::TopBorder ||
            frame->enabledBorders & FrameSvg::BottomBorder) {

            if (q->hasElement(prefix + "top") &&
                frame->enabledBorders & FrameSvg::TopBorder) {
                q->paint(&p, QRect(contentLeft, topOffset, contentWidth, frame->topHeight), prefix + "top");
            }

            if (q->hasElement(prefix + "bottom") &&
                frame->enabledBorders & FrameSvg::BottomBorder) {
                q->paint(&p, QRect(contentLeft, bottomOffset, contentWidth, frame->bottomHeight), prefix + "bottom");
            }

        }
    } else {
        if (q->hasElement(prefix + "left") &&
            frame->enabledBorders & FrameSvg::LeftBorder) {
            QPixmap left(frame->leftWidth, leftHeight);
            left.fill(Qt::transparent);

            QPainter sidePainter(&left);
            sidePainter.setCompositionMode(QPainter::CompositionMode_Source);
            q->paint(&sidePainter, QRect(QPoint(0, 0), left.size()), prefix + "left");

            p.drawTiledPixmap(QRect(leftOffset, contentTop, frame->leftWidth, contentHeight), left);
        }

        if (q->hasElement(prefix + "right") && frame->enabledBorders & FrameSvg::RightBorder) {
            QPixmap right(frame->rightWidth, leftHeight);
            right.fill(Qt::transparent);

            QPainter sidePainter(&right);
            sidePainter.setCompositionMode(QPainter::CompositionMode_Source);
            q->paint(&sidePainter, QRect(QPoint(0, 0), right.size()), prefix + "right");

            p.drawTiledPixmap(QRect(rightOffset, contentTop, frame->rightWidth, contentHeight), right);
        }

        if (q->hasElement(prefix + "top") && frame->enabledBorders & FrameSvg::TopBorder) {
            QPixmap top(topWidth, frame->topHeight);
            top.fill(Qt::transparent);

            QPainter sidePainter(&top);
            sidePainter.setCompositionMode(QPainter::CompositionMode_Source);
            q->paint(&sidePainter, QRect(QPoint(0, 0), top.size()), prefix + "top");

            p.drawTiledPixmap(QRect(contentLeft, topOffset, contentWidth, frame->topHeight), top);
        }

        if (q->hasElement(prefix + "bottom") && frame->enabledBorders & FrameSvg::BottomBorder) {
            QPixmap bottom(topWidth, frame->bottomHeight);
            bottom.fill(Qt::transparent);

            QPainter sidePainter(&bottom);
            sidePainter.setCompositionMode(QPainter::CompositionMode_Source);
            q->paint(&sidePainter, QRect(QPoint(0, 0), bottom.size()), prefix + "bottom");

            p.drawTiledPixmap(QRect(contentLeft, bottomOffset, contentWidth, frame->bottomHeight), bottom);
        }
    }

    theme->insertIntoCache(id, frame->cachedBackground);
}

void FrameSvgPrivate::updateSizes()
{
    //kDebug() << "!!!!!!!!!!!!!!!!!!!!!! updating sizes" << prefix;
    FrameData *frame = frames[prefix];
    Q_ASSERT(frame);

    frame->cachedBackground = QPixmap();
    frame->cachedMask = QPixmap();

    if (frame->enabledBorders & FrameSvg::TopBorder) {
        frame->topHeight = q->elementSize(prefix + "top").height();

        if (q->hasElement(prefix + "hint-top-margin")) {
            frame->topMargin = q->elementSize(prefix + "hint-top-margin").height();
        } else {
            frame->topMargin = frame->topHeight;
        }
    } else {
        frame->topMargin = frame->topHeight = 0;
    }

    if (frame->enabledBorders & FrameSvg::LeftBorder) {
        frame->leftWidth = q->elementSize(prefix + "left").width();

        if (q->hasElement(prefix + "hint-left-margin")) {
            frame->leftMargin = q->elementSize(prefix + "hint-left-margin").height();
        } else {
            frame->leftMargin = frame->leftWidth;
        }
    } else {
        frame->leftMargin = frame->leftWidth = 0;
    }

    if (frame->enabledBorders & FrameSvg::RightBorder) {
        frame->rightWidth = q->elementSize(prefix + "right").width();

        if (q->hasElement(prefix + "hint-right-margin")) {
            frame->rightMargin = q->elementSize(prefix + "hint-right-margin").height();
        } else {
            frame->rightMargin = frame->rightWidth;
        }
    } else {
        frame->rightMargin = frame->rightWidth = 0;
    }

    if (frame->enabledBorders & FrameSvg::BottomBorder) {
        frame->bottomHeight = q->elementSize(prefix + "bottom").height();

        if (q->hasElement(prefix + "hint-bottom-margin")) {
            frame->bottomMargin = q->elementSize(prefix + "hint-bottom-margin").height();
        } else {
            frame->bottomMargin = frame->bottomHeight;
        }
    } else {
        frame->bottomMargin = frame->bottomHeight = 0;
    }

    //since it's rectangular, topWidth and bottomWidth must be the same
    frame->tileCenter = q->hasElement("hint-tile-center");
    frame->noBorderPadding = q->hasElement("hint-no-border-padding");
    frame->stretchBorders = q->hasElement("hint-stretch-borders");
}

void FrameSvgPrivate::updateNeeded()
{
    q->clearCache();
    updateSizes();
}

void FrameSvgPrivate::updateAndSignalSizes()
{
    updateSizes();
    emit q->repaintNeeded();
}

} // Plasma namespace

#include "framesvg.moc"
