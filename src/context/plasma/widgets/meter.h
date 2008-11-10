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

#ifndef PLASMA_METER_H
#define PLASMA_METER_H

#include <plasma/plasma_export.h>
#include <plasma/dataengine.h>
#include <QtGui/QGraphicsWidget>

namespace Plasma
{

class MeterPrivate;

/**
 * @class Meter plasma/widgets/meter.h <Plasma/Widgets/Meter>
 *
 * @short Provides generic meter widget for Plasma
 *
 * Analog and bar meters are supported.
 *
 * Svgs can have following ids:
 * - background: Drawn first to the bottom
 *       background can be a FrameSvg
 * - label0, label1, ...: Rectangles mark the label places
 * - bar: Bar for the bar meter
 *      can be replaced with bar-active and bar-inactive FrameSvg
 * - pointer: Pointer for analog meter
 * - rotatecenter: Marks the place of pointer rotation center
 * - rotateminmax: Width and height of this object are the Min and Max rotate
 *                 angles for the pointer
 * - foreground: Is drawn to top
 *
 * @author Petri Damstén
 */

class PLASMA_EXPORT Meter : public QGraphicsWidget
{
    Q_OBJECT
    Q_ENUMS(MeterType)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(QString svg READ svg WRITE setSvg)
    Q_PROPERTY(MeterType meterType READ meterType WRITE setMeterType)

public:
    /**
     * Meter types enum
     */
    enum MeterType {
        /** Horizontal bar meter (like thermometer). */
        BarMeterHorizontal,
        /** Vertical bar meter (like thermometer). */
        BarMeterVertical,
        /** Analog meter (like tachometer). */
        AnalogMeter
    };

    /**
     * Constructor
     * @param parent the QGraphicsItem this meter is parented to.
     * @param parent the QObject this meter is parented to.
     */
    explicit Meter(QGraphicsItem *parent = 0);

    /**
     * Destructor
     */
    ~Meter();

    /**
     * Set maximum value for the meter
     */
    void setMaximum(int maximum);

    /**
     * @return maximum value for the meter
     */
    int maximum() const;

    /**
     * Set minimum value for the meter
     */
    void setMinimum(int minimum);

    /**
     * @return minimum value for the meter
     */
    int minimum() const;

    /**
     * Set value for the meter
     */
    void setValue(int value);

    /**
     * @return value for the meter
     */
    int value() const;

    /**
     * Set svg file name
     */
    void setSvg(const QString &svg);

    /**
     * @return svg file name
     */
    QString svg() const;

    /**
     * Set meter type. Note: setSvg gets called automatically with the proper
     * default values if svg is not set.
     */
    void setMeterType(MeterType type);

    /**
     * @return meter type
     */
    MeterType meterType() const;

    /**
     * Set text label for the meter
     * @param index label index.
     * @param text text for the label.
     */
    void setLabel(int index, const QString &text);

    /**
     * @param index label index
     * @return text label for the meter
     */
    QString label(int index) const;

    /**
     * Set text label color for the meter
     * @param index label index.
     * @param text color for the label.
     */
    void setLabelColor(int index, const QColor &color);

    /**
     * @param index label index
     * @return text label color for the meter
     */
    QColor labelColor(int index) const;

    /**
     * Set text label font for the meter
     * @param index label index.
     * @param text font for the label.
     */
    void setLabelFont(int index, const QFont &font);

    /**
     * @param index label index
     * @return text label font for the meter
     */
    QFont labelFont(int index) const;

    /**
     * Set text label alignment for the meter
     * @param index label index.
     * @param text alignment for the label.
     */
    void setLabelAlignment(int index, const Qt::Alignment alignment);

    /**
     * @param index label index
     * @return text label alignment for the meter
     */
    Qt::Alignment labelAlignment(int index) const;

public Q_SLOTS:
    /**
     * Used when connecting to a DataEngine
     */
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

protected:
    /**
     * Reimplemented from Plasma::Widget
     */
    virtual void paint(QPainter *p,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget = 0);

private:
    MeterPrivate *const d;
};

} // End of namepace

#endif
