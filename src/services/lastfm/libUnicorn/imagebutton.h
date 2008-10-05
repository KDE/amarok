/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H

#include "UnicornDllExportMacro.h"

#include <QLabel>
#include <QPixmap>
#include <QUrl>

class UNICORN_DLLEXPORT ImageButton : public QLabel
{
    Q_OBJECT

public:
    ImageButton( QWidget* parent = 0 );

    void setPixmap( const QPixmap& pixmap );
    void setPixmapDown( const QPixmap& pixmap ) { m_down = pixmap; }
    void setPixmapHover( const QPixmap& pixmap ) { m_hover = pixmap; }
    void setPixmapDisabled( const QPixmap& pixmap ) { m_disabled = pixmap; }

    void setEnabled( bool enabled );
    void setHoverCursor( const QCursor& cursor ) { setCursor( cursor ); }

    /** causes clicking the button to open the URL */
    void setUrl( const QUrl& url ) { m_url = url; }
    QUrl getUrl() { return m_url; }

signals:
    void clicked();
    void urlHovered( const QString& url );

protected:
    void enterEvent( QEvent* );
    void leaveEvent( QEvent* );
    void mousePressEvent( QMouseEvent* );
    void mouseReleaseEvent( QMouseEvent* );
    void mouseMoveEvent( QMouseEvent* );

private:
    QPixmap m_normal;
    QPixmap m_down;
    QPixmap m_hover;
    QPixmap m_disabled;
    QUrl m_url;
    bool m_enabled;
};

#endif
