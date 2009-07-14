/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BOOKMARK_APPLET_H
#define BOOKMARK_APPLET_H

#include "Applet.h"
#include "DataEngine.h"
#include "Svg.h"
#include "widgets/TrackWidget.h"
#include "Meta.h"
#include "BookmarkManagerWidgetProxy.h"


#include <QAction>
#include <QList>
#include <QGraphicsProxyWidget>

class RatingWidget;
class QCheckBox;
class QGraphicsPixmapItem;
class QHBoxLayout;
class QLabel;
class QSpinBox;

namespace Plasma { class DataEngine; }

static const int MAX_PLAYED_TRACKS = 5;

class Bookmark : public Context::Applet
{
    Q_OBJECT

public:
    Bookmark( QObject* parent, const QVariantList& args );
    ~Bookmark();

    virtual void init();

    virtual void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

protected:
    virtual void constraintsEvent( Plasma::Constraints constraints );

private slots:
    void paletteChanged( const QPalette & palette );

private:
    
    BookmarkManagerWidgetProxy* m_bookmarkWidget;
};

K_EXPORT_AMAROK_APPLET( bookmark, Bookmark )

#endif
