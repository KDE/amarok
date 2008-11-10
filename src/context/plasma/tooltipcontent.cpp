/*
 * Copyright 2008 by Aaron Seigo <aseigo@kde.org>
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

#include "tooltipcontent.h"

#include <KIconLoader>

namespace Plasma
{

class ToolTipContentPrivate
{
public:
    ToolTipContentPrivate()
      : windowToPreview(0),
        autohide(true)
    {
    }

    QString mainText;
    QString subText;
    QPixmap image;
    WId windowToPreview;
    bool autohide;
};

ToolTipContent::ToolTipContent()
    : d(new ToolTipContentPrivate)
{
}

ToolTipContent::ToolTipContent(const ToolTipContent &other)
    : d(new ToolTipContentPrivate(*other.d))
{
}

ToolTipContent::~ToolTipContent()
{
    delete d;
}

ToolTipContent &ToolTipContent::operator=(const ToolTipContent &other)
{
    *d = *other.d;
    return *this;
}

ToolTipContent::ToolTipContent(const QString &mainText,
                               const QString &subText,
                               const QPixmap &image)
    : d(new ToolTipContentPrivate)
{
    d->mainText = mainText;
    d->subText = subText;
    d->image = image;
}

ToolTipContent::ToolTipContent(const QString &mainText,
                               const QString &subText,
                               const QIcon &icon)
    : d(new ToolTipContentPrivate)
{
    d->mainText = mainText;
    d->subText = subText;
    d->image = icon.pixmap(IconSize(KIconLoader::Desktop));
}

bool ToolTipContent::isEmpty() const
{
    return d->mainText.isEmpty() &&
           d->subText.isEmpty() &&
           d->image.isNull() &&
           d->windowToPreview == 0;
}

void ToolTipContent::setMainText(const QString &text)
{
    d->mainText = text;
}

QString ToolTipContent::mainText() const
{
    return d->mainText;
}

void ToolTipContent::setSubText(const QString &text)
{
    d->subText = text;
}

QString ToolTipContent::subText() const
{
    return d->subText;
}

void ToolTipContent::setImage(const QPixmap &image)
{
    d->image = image;
}

void ToolTipContent::setImage(const QIcon &icon)
{
    d->image = icon.pixmap(IconSize(KIconLoader::Desktop));
}

QPixmap ToolTipContent::image() const
{
    return d->image;
}

void ToolTipContent::setWindowToPreview(WId id)
{
    d->windowToPreview = id;
}

WId ToolTipContent::windowToPreview() const
{
    return d->windowToPreview;
}

void ToolTipContent::setAutohide(bool autohide)
{
    d->autohide = autohide;
}

bool ToolTipContent::autohide() const
{
    return d->autohide;
}

} // namespace Plasma


