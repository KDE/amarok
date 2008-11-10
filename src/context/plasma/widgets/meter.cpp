/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
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

#include "meter.h"
#include "plasma/framesvg.h"
#include <cmath>
#include <kdebug.h>
#include <QPainter>

namespace Plasma {

class MeterPrivate
{
public:
    MeterPrivate(Meter *m)
        : minimum(0),
          maximum(100),
          value(0),
          meterType(Meter::AnalogMeter),
          image(0),
          minrotate(0),
          maxrotate(360),
          meter(m) {}

    void paint(QPainter *p, const QString &elementID)
    {
        if (image->hasElement(elementID)) {
            QRectF elementRect = image->elementRect(elementID);
            image->paint(p, elementRect, elementID);
        }
    }

    void text(QPainter *p, int index)
    {
        QString elementID = QString("label%1").arg(index);
        QString text = labels[index];

        if (image->hasElement(elementID)) {
            QRectF elementRect = image->elementRect(elementID);
            Qt::Alignment align = Qt::AlignCenter;

            if (colors.count() > index) {
                p->setPen(QPen(colors[index]));
            }
            if (fonts.count() > index) {
                p->setFont(fonts[index]);
            }
            if (alignments.count() > index) {
                align = alignments[index];
            }
            if (elementRect.width() > elementRect.height()) {
                p->drawText(elementRect, align, text);
            } else {
                p->save();
                QPointF rotateCenter(
                        elementRect.left() + elementRect.width() / 2,
                        elementRect.top() + elementRect.height() / 2);
                p->translate(rotateCenter);
                p->rotate(-90);
                p->translate(elementRect.height() / -2,
                             elementRect.width() / -2);
                QRectF r(0, 0, elementRect.height(), elementRect.width());
                p->drawText(r, align, text);
                p->restore();
            }
        }
    }

    QRectF barRect()
    {
        if (labels.count() > 0) {
            return image->elementRect("background");
        } else {
            return QRectF(QPoint(0,0), meter->size());
        }
    }

    void paintBackground(QPainter *p)
    {
        //be retrocompatible with themes for kde <= 4.1
        if (image->hasElement("background-center")) {
            QRectF elementRect = barRect();
            QSize imageSize = image->size();
            image->resize();

            image->setElementPrefix("background");
            image->resizeFrame(elementRect.size());
            image->paintFrame(p, elementRect.topLeft());
            image->resize(imageSize);

            paintBar(p, "bar-inactive");
        } else {
            paint(p, "background");
        }
    }

    void paintBar(QPainter *p, const QString &prefix)
    {
        QRectF elementRect = barRect();
        QSize imageSize = image->size();
        image->resize();
        QSize tileSize = image->elementSize("bar-active-center");

        if (elementRect.width() > elementRect.height()) {
            qreal ratio = tileSize.height() / tileSize.width();
            int numTiles = elementRect.width()/(elementRect.height()/ratio);
            tileSize = QSize(elementRect.width()/numTiles, elementRect.height());

            QPoint center = elementRect.center().toPoint();
            elementRect.setWidth(tileSize.width()*numTiles);
            elementRect.moveCenter(center);
        } else {
            qreal ratio = tileSize.width() / tileSize.height();
            int numTiles = elementRect.height()/(elementRect.width()/ratio);
            tileSize = QSize(elementRect.width(), elementRect.height()/numTiles);

            QPoint center = elementRect.center().toPoint();
            elementRect.setHeight(tileSize.height()*numTiles);
            elementRect.moveCenter(center);
        }

        image->setElementPrefix(prefix);
        image->resizeFrame(tileSize);
        p->drawTiledPixmap(elementRect, image->framePixmap());
        image->resize(imageSize);
    }

    void paintForeground(QPainter *p)
    {
        for (int i = 0; i < labels.count(); ++i) {
            text(p, i);
        }

        paint(p, "foreground");
    }

    void setSizePolicyAndPreferredSize()
    {
        switch (meterType) {
            case Meter::BarMeterHorizontal:
                meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                break;
            case Meter::BarMeterVertical:
                meter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                break;
            case Meter::AnalogMeter:
            default:
                meter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                break;
        }
        if (image) {
            meter->setPreferredSize(image->size());
        } else {
            meter->setPreferredSize(QSizeF(30, 30));
        }
    }

    int minimum;
    int maximum;
    int value;
    QStringList labels;
    QList<Qt::Alignment> alignments;
    QList<QColor> colors;
    QList<QFont> fonts;
    QString svg;
    Meter::MeterType meterType;
    Plasma::FrameSvg *image;
    int minrotate;
    int maxrotate;
    Meter *meter;
};

Meter::Meter(QGraphicsItem *parent) :
        QGraphicsWidget(parent),
        d(new MeterPrivate(this))
{
    d->setSizePolicyAndPreferredSize();
}

Meter::~Meter()
{
    delete d;
}

void Meter::setMaximum(int maximum)
{
    d->maximum = maximum;
}

int Meter::maximum() const
{
    return d->maximum;
}

void Meter::setMinimum(int minimum)
{
    d->minimum = minimum;
}

int Meter::minimum() const
{
    return d->minimum;
}

void Meter::setValue(int value)
{
    d->value = value;
    update();
}

int Meter::value() const
{
    return d->value;
}

void Meter::setLabel(int index, const QString &text)
{
    while (d->labels.count() <= index) {
        d->labels << QString();
    }
    d->labels[index] = text;
}

QString Meter::label(int index) const
{
    return d->labels[index];
}

void Meter::setLabelColor(int index, const QColor &color)
{
    while (d->colors.count() <= index) {
        d->colors << color;
    }
    d->colors[index] = color;
}

QColor Meter::labelColor(int index) const
{
    return d->colors[index];
}

void Meter::setLabelFont(int index, const QFont &font)
{
    while (d->fonts.count() <= index) {
        d->fonts << font;
    }
    d->fonts[index] = font;
}

QFont Meter::labelFont(int index) const
{
    return d->fonts[index];
}

void Meter::setLabelAlignment(int index, Qt::Alignment alignment)
{
    while (d->alignments.count() <= index) {
        d->alignments << alignment;
    }
    d->alignments[index] = alignment;
}

Qt::Alignment Meter::labelAlignment(int index) const
{
    return d->alignments[index];
}

void Meter::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(sourceName)

    foreach (const QVariant &v, data) {
        if (v.type() == QVariant::Int ||
            v.type() == QVariant::UInt ||
            v.type() == QVariant::LongLong ||
            v.type() == QVariant::ULongLong) {
            setValue(v.toInt());
            return;
        }
    }
}

void Meter::setSvg(const QString &svg)
{
    d->svg = svg;
    delete d->image;
    d->image = new Plasma::FrameSvg(this);
    d->image->setImagePath(svg);
    // To create renderer and get default size
    d->image->resize();
    d->setSizePolicyAndPreferredSize();
    if (d->image->hasElement("rotateminmax")) {
        QRectF r = d->image->elementRect("rotateminmax");
        d->minrotate = (int)r.height();
        d->maxrotate = (int)r.width();
    }
}

QString Meter::svg() const
{
    return d->svg;
}

void Meter::setMeterType(MeterType meterType)
{
    d->meterType = meterType;
    if (d->svg.isEmpty()) {
        if (meterType == BarMeterHorizontal) {
            setSvg("widgets/bar_meter_horizontal");
        } else if (meterType == BarMeterVertical) {
            setSvg("widgets/bar_meter_vertical");
        } else if (meterType == AnalogMeter) {
            setSvg("widgets/analog_meter");
        }
    }
    d->setSizePolicyAndPreferredSize();
}

Meter::MeterType Meter::meterType() const
{
    return d->meterType;
}

void Meter::paint(QPainter *p,
                  const QStyleOptionGraphicsItem *option,
                  QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!d->image) {
        return;
    }

    QRectF rect(QPointF(0, 0), size());
    QRectF clipRect;
    qreal percentage = 0.0;
    qreal angle = 0.0;
    QPointF rotateCenter;
    QSize intSize = QSize((int)size().width(), (int)size().height());

    if (intSize != d->image->size()) {
        d->image->resize(intSize);
    }

    if (d->maximum != d->minimum) {
        percentage = (qreal)d->value / (d->maximum - d->minimum);
    }
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    switch (d->meterType) {
    case BarMeterHorizontal:
    case BarMeterVertical:
        d->paintBackground(p);

        p->save();
        clipRect = d->barRect();
        if (clipRect.width() > clipRect.height()) {
            clipRect.setWidth(clipRect.width() * percentage);
        } else {
            qreal bottom = clipRect.bottom();
            clipRect.setHeight(clipRect.height() * percentage);
            clipRect.moveBottom(bottom);
        }
        p->setClipRect(clipRect);
        //be retrocompatible
        if (d->image->hasElement("bar-active-center")) {
            d->paintBar(p, "bar-active");
        } else {
            d->paint(p, "bar");
        }
        p->restore();

        d->paintForeground(p);
        break;
    case AnalogMeter:
        d->paintBackground(p);

        p->save();
        if (d->image->hasElement("rotatecenter")) {
            QRectF r = d->image->elementRect("rotatecenter");
            rotateCenter = QPointF(r.left() + r.width() / 2,
                                   r.top() + r.height() / 2);
        } else {
            rotateCenter = QPointF(rect.width() / 2, rect.height() / 2);
        }
        angle = percentage * (d->maxrotate - d->minrotate) + d->minrotate;

        p->translate(rotateCenter);
        p->rotate(angle);
        p->translate(-1 * rotateCenter);
        d->paint(p, "pointer");
        p->restore();

        d->paintForeground(p);
        break;
    }
}

} // End of namepace

#include "meter.moc"
