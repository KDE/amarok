/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SERVICE_INFO_APPLET_H
#define SERVICE_INFO_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"
#include "core/playlists/Playlist.h"

#include <KDialog>

#include <QGraphicsProxyWidget>

class KGraphicsWebView;

class InfoApplet : public Context::Applet
{
    Q_OBJECT

public:
    InfoApplet( QObject* parent, const QVariantList& args );
    virtual ~InfoApplet();

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

public Q_SLOTS:
    virtual void init();
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

private Q_SLOTS:
    void linkClicked( const QUrl & url );

private:
    KGraphicsWebView *m_webView;
    bool m_initialized;

    static QString s_defaultHtml;
};

AMAROK_EXPORT_APPLET( info, InfoApplet )

#endif
