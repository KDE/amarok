/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTGRAPHICSITEM_H
#define AMAROK_PLAYLISTGRAPHICSITEM_H


#include "meta.h"
#include <QGraphicsItem>

class QFontMetricsF;

namespace Playlist
{
    class GraphicsItem : public QGraphicsItem
    {
        class ActiveItems;

        public:
            GraphicsItem();
            ~GraphicsItem();
            void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
            QRectF boundingRect() const;
            void setupItem();
            static qreal height() { return s_height; }
            void refresh();
            void play();
       
        protected:
            void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            void dropEvent( QGraphicsSceneDragDropEvent * event );
            void mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event );
            void mousePressEvent( QGraphicsSceneMouseEvent* event );
            void mouseMoveEvent( QGraphicsSceneMouseEvent* event );
            void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
        
        private:
            void init( Meta::TrackPtr track );
            void resize( Meta::TrackPtr track, int totalWidth );
            int getRow() const { return int( ( mapToScene( 0.0, 0.0 ).y() ) / s_height ); }

            ActiveItems* m_items;
            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;
            static       qreal s_height;
            static QFontMetricsF* s_fm;
            Meta::TrackPtr m_track;
            double m_verticalOffset;
    };

}
#endif

