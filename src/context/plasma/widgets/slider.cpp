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

#include "slider.h"

#include <QSlider>
#include <QPainter>
#include <QStyleOptionSlider>

#include <KMimeType>

#include "theme.h"
#include "framesvg.h"

namespace Plasma
{

class SliderPrivate
{
public:
    SliderPrivate()
    {
    }

    ~SliderPrivate()
    {
    }

    Plasma::FrameSvg *background;
    Plasma::FrameSvg *handle;
};

Slider::Slider(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new SliderPrivate)
{
    QSlider *native = new QSlider;

    connect(native, SIGNAL(sliderMoved(int)), this, SIGNAL(sliderMoved(int)));
    connect(native, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));

    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);

    d->background = new Plasma::FrameSvg(this);
    d->background->setImagePath("widgets/frame");
    d->background->setElementPrefix("sunken");

    d->handle = new Plasma::FrameSvg(this);
    d->handle->setImagePath("widgets/button");
    d->handle->setElementPrefix("normal");
}

Slider::~Slider()
{
    delete d;
}

void Slider::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    if (!styleSheet().isNull()) {
        QGraphicsProxyWidget::paint(painter, option, widget);
        return;
    }

    QSlider *slider = nativeWidget();
    QStyle *style = slider->style();
    QStyleOptionSlider sliderOpt;
    sliderOpt.initFrom(slider);

    //init the other stuff in the slider, taken from initStyleOption()
    sliderOpt.subControls = QStyle::SC_None;
    sliderOpt.activeSubControls = QStyle::SC_None;
    sliderOpt.orientation = slider->orientation();
    sliderOpt.maximum = slider->maximum();
    sliderOpt.minimum = slider->minimum();
    sliderOpt.tickPosition = (QSlider::TickPosition)slider->tickPosition();
    sliderOpt.tickInterval = slider->tickInterval();
    sliderOpt.upsideDown = (slider->orientation() == Qt::Horizontal) ?
                     (slider->invertedAppearance() != (sliderOpt.direction == Qt::RightToLeft))
                     : (!slider->invertedAppearance());
    sliderOpt.direction = Qt::LeftToRight; // we use the upsideDown option instead
    sliderOpt.sliderPosition = slider->sliderPosition();
    sliderOpt.sliderValue = slider->value();
    sliderOpt.singleStep = slider->singleStep();
    sliderOpt.pageStep = slider->pageStep();
    if (slider->orientation() == Qt::Horizontal) {
        sliderOpt.state |= QStyle::State_Horizontal;
    }

    QRect backgroundRect =
        style->subControlRect(QStyle::CC_Slider, &sliderOpt, QStyle::SC_SliderGroove, slider);
    d->background->resizeFrame(backgroundRect.size());
    d->background->paintFrame(painter, backgroundRect.topLeft());

    //Thickmarks
    if (sliderOpt.tickPosition != QSlider::NoTicks) {
        sliderOpt.subControls = QStyle::SC_SliderTickmarks;
        sliderOpt.palette.setColor(
            QPalette::WindowText, Plasma::Theme::defaultTheme()->color(Theme::TextColor));
        style->drawComplexControl(QStyle::CC_Slider, &sliderOpt, painter, slider);
    }

    QRect handleRect =
        style->subControlRect(QStyle::CC_Slider, &sliderOpt, QStyle::SC_SliderHandle, slider);
    d->handle->resizeFrame(handleRect.size());
    d->handle->paintFrame(painter, handleRect.topLeft());
}

void Slider::setMaximum(int max)
{
    static_cast<QSlider*>(widget())->setMaximum(max);
}

int Slider::maximum() const
{
    return static_cast<QSlider*>(widget())->maximum();
}

void Slider::setMinimum(int min)
{
    static_cast<QSlider*>(widget())->setMinimum(min);
}

int Slider::minimum() const
{
    return static_cast<QSlider*>(widget())->minimum();
}

void Slider::setRange(int min, int max)
{
    static_cast<QSlider*>(widget())->setRange(min, max);
}

void Slider::setValue(int value)
{
    static_cast<QSlider*>(widget())->setValue(value);
}

int Slider::value() const
{
    return static_cast<QSlider*>(widget())->value();
}

void Slider::setOrientation(Qt::Orientation orientation)
{
    static_cast<QSlider*>(widget())->setOrientation(orientation);
}

Qt::Orientation Slider::orientation() const
{
    return static_cast<QSlider*>(widget())->orientation();
}

void Slider::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString Slider::styleSheet()
{
    return widget()->styleSheet();
}

QSlider *Slider::nativeWidget() const
{
    return static_cast<QSlider*>(widget());
}

} // namespace Plasma

#include <slider.moc>

