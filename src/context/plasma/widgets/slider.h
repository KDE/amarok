/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_SLIDER_H
#define PLASMA_SLIDER_H

#include <QtGui/QGraphicsProxyWidget>

#include <plasma/plasma_export.h>

class QSlider;

namespace Plasma
{

class SliderPrivate;

/**
 * @class Slider plasma/widgets/slider.h <Plasma/Widgets/Slider>
 *
 * @short Provides a plasma-themed QSlider.
 */
class PLASMA_EXPORT Slider : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(int maximum READ maximum WRITE setMinimum)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(QSlider *nativeWidget READ nativeWidget)

public:
    explicit Slider(QGraphicsWidget *parent = 0);
    ~Slider();

    /**
     * @return the maximum value
     */
    int maximum() const;

    /**
     * @return the minimum value
     */
    int minimum() const;

    /**
     * @return the current value
     */
    int value() const;

    /**
     * @return the orientation of the slider
     */
    Qt::Orientation orientation() const;

    /**
     * Sets the stylesheet used to control the visual display of this Slider
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this Slider
     */
    QSlider *nativeWidget() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public Q_SLOTS:
    /**
     * Sets the maximum value the slider can take.
     */
    void setMaximum(int maximum);

    /**
     * Sets the minimum value the slider can take.
     */
    void setMinimum(int minimum);

    /**
     * Sets the minimum and maximum values the slider can take.
     */
    void setRange(int minimum, int maximum);

    /**
     * Sets the value of the slider.
     *
     * If it is outside the range specified by minimum() and maximum(),
     * it will be adjusted to fit.
     */
    void setValue(int value);

    /**
     * Sets the orientation of the slider.
     */
    void setOrientation(Qt::Orientation orientation);

Q_SIGNALS:
    /**
     * This signal is emitted when the user drags the slider.
     *
     * In fact, it is emitted whenever the sliderMoved(int) signal
     * of QSlider would be emitted.  See the Qt documentation for
     * more information.
     */
    void sliderMoved(int value);

    /**
     * This signal is emitted when the slider value has changed,
     * with the new slider value as argument.
     */
    void valueChanged(int value);

private:
    SliderPrivate * const d;
};

} // namespace Plasma

#endif // multiple inclusion guard
