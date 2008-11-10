/*
 *   Copyright 2007 by Dan Meltzer <hydrogen@notyetimplemented.com>
 *   Copyright (C) 2008 by Alexis Ménard <darktears31@gmail.com>
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

#ifndef PLASMA_WINDOWPREVIEW_P_H
#define PLASMA_WINDOWPREVIEW_P_H

#include <QWidget> // base class
#include <QSize> // stack allocated

namespace Plasma {

/**
 * @internal
 *
 * A widget which reserves area for window preview and sets hints on the toplevel
 * tooltip widget that tells KWin to render the preview in this area. This depends
 * on KWin's TaskbarThumbnail compositing effect (which is automatically detected).
 */
class WindowPreview : public QWidget
{
    Q_OBJECT

public:
    static bool previewsAvailable();

    WindowPreview(QWidget *parent = 0);

    void setWindowId(WId w);
    WId windowId() const;
    void setInfo();
    virtual QSize sizeHint() const;

private:
    void readWindowSize() const;

    WId id;
    mutable QSize windowSize;
};

} // namespace Plasma

#endif // PLASMA_WINDOWPREVIEW_P_H

