/*
 *   Copyright 2005 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Andrew Lake <jamboarder@yahoo.com>
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

#ifndef PLASMA_PAINTUTILS_H
#define PLASMA_PAINTUTILS_H

#include <QtGui/QGraphicsItem>
#include <QtGui/QPainterPath>

#include <plasma/plasma_export.h>
#include "theme.h"

/** @headerfile plasma/paintutils.h <Plasma/PaintUtils> */

namespace Plasma
{

/**
 *  Namespace for all Image Effects specific to Plasma
 **/
namespace PaintUtils
{

/**
 * Creates a blurred shadow of the supplied image.
 */
PLASMA_EXPORT void shadowBlur(QImage &image, int radius, const QColor &color);

/**
 * Returns a pixmap containing text with blurred shadow.
 * Text and shadow colors default to Plasma::Theme colors.
 */
PLASMA_EXPORT QPixmap shadowText(QString text,
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor),
    QColor shadowColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor),
    QPoint offset = QPoint(1,1),
    int radius = 2);

/**
 * Returns a nicely rounded rectanglular path for painting.
 */
PLASMA_EXPORT QPainterPath roundedRectangle(const QRectF &rect, qreal radius);

/**
 * Blends a pixmap into another
 */
PLASMA_EXPORT QPixmap transition(const QPixmap &from, const QPixmap &to, qreal amount);

} // PaintUtils namespace

} // Plasma namespace

#endif
