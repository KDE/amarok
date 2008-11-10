/*
 *   Copyright © 2008 Fredrik Höglund <fredrik@kde.org>
 *   Copyright © 2008 Marco Martin <notmart@gmail.com>
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

#include "style.h"

#include <QPainter>
#include <QStyleOptionComplex>

#include <KDebug>

#include <plasma/framesvg.h>

namespace Plasma {

class StylePrivate
{
public:
    StylePrivate()
        : scrollbar(0)
    {
    }

    ~StylePrivate()
    {
    }

    Plasma::FrameSvg *scrollbar;
};

Style::Style()
     : QCommonStyle(),
       d(new StylePrivate)
{
    d->scrollbar = new Plasma::FrameSvg(this);
    d->scrollbar->setImagePath("widgets/scrollbar");
    d->scrollbar->setCacheAllRenderedFrames(true);
}

Style::~Style()
{
    delete d;
}

void Style::drawComplexControl(ComplexControl control,
                               const QStyleOptionComplex *option,
                               QPainter *painter,
                               const QWidget *widget) const
{
    if (control != CC_ScrollBar) {
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool sunken = option->state & State_Sunken;
    const QStyleOptionSlider *scrollOption = qstyleoption_cast<const QStyleOptionSlider *>(option);
    QString prefix;

    if (option->state & State_MouseOver) {
        prefix= "mouseover-";
    }

    QRect subLine;
    QRect addLine;
    if (scrollOption && scrollOption->orientation == Qt::Horizontal) {
        subLine = d->scrollbar->elementRect(prefix + "arrow-left").toRect();
        addLine = d->scrollbar->elementRect(prefix + "arrow-right").toRect();
    } else {
        subLine = d->scrollbar->elementRect(prefix + "arrow-up").toRect();
        addLine = d->scrollbar->elementRect(prefix + "arrow-down").toRect();
    }

    subLine.moveCenter(subControlRect(control, option, SC_ScrollBarSubLine, widget).center());
    addLine.moveCenter(subControlRect(control, option, SC_ScrollBarAddLine, widget).center());

    const QRect slider =
        subControlRect(control, option, SC_ScrollBarSlider, widget).adjusted(1, 0, -1, 0);

    d->scrollbar->setElementPrefix("background");
    d->scrollbar->resizeFrame(option->rect.size());
    d->scrollbar->paintFrame(painter);

    if (sunken && scrollOption && scrollOption->activeSubControls & SC_ScrollBarSlider) {
        d->scrollbar->setElementPrefix("sunken-slider");
    } else {
        d->scrollbar->setElementPrefix(prefix + "slider");
    }

    d->scrollbar->resizeFrame(slider.size());
    d->scrollbar->paintFrame(painter, slider.topLeft());

    if (scrollOption && scrollOption->orientation == Qt::Horizontal) {
        if (sunken && scrollOption->activeSubControls & SC_ScrollBarAddLine) {
            d->scrollbar->paint(painter, addLine, "sunken-arrow-right");
        } else {
            d->scrollbar->paint(painter, addLine, prefix + "arrow-right");
        }

        if (sunken && scrollOption->activeSubControls & SC_ScrollBarSubLine) {
            d->scrollbar->paint(painter, subLine, "sunken-arrow-left");
        } else {
            d->scrollbar->paint(painter, subLine, prefix + "arrow-left");
        }
    } else {
        if (sunken && scrollOption && scrollOption->activeSubControls & SC_ScrollBarAddLine) {
            d->scrollbar->paint(painter, addLine, "sunken-arrow-down");
        } else {
            d->scrollbar->paint(painter, addLine, prefix + "arrow-down");
        }

        if (sunken && scrollOption && scrollOption->activeSubControls & SC_ScrollBarSubLine) {
            d->scrollbar->paint(painter, subLine, "sunken-arrow-up");
        } else {
            d->scrollbar->paint(painter, subLine, prefix + "arrow-up");
        }
    }

    painter->restore();
}

}

