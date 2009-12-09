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

#include "context/widgets/AmarokWebView.h"

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/Svg.h>

#include "meta/XSPFPlaylist.h"
#include "meta/Playlist.h"

#include <Plasma/FrameSvg>
 
#include <KDialog>

#include <QGraphicsProxyWidget>
#include <qwebview.h>


class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;

class InfoApplet : public Context::Applet
{
    Q_OBJECT

public:
    InfoApplet( QObject* parent, const QVariantList& args );
    virtual ~InfoApplet();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

private slots:
    void linkClicked( const QUrl & url );

private:
    AmarokWebView * m_webView;
    bool m_initialized;
    Meta::XSPFPlaylist * m_currentPlaylist;

    static QString s_defaultHtml;
};

K_EXPORT_AMAROK_APPLET( info, InfoApplet )

#endif
