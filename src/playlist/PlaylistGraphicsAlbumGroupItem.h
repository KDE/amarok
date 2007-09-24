/***************************************************************************
 * copyright     : (C) 2007 Ian Monroe <ian@monroe.nu>                     *
 *                 (C) 2007 Nikolaj Hals Nielsen <nhnFreespirit@gmail.com  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTGRAPHICSALBUMGROUPITEM_H
#define AMAROK_PLAYLISTGRAPHICSALBUMGROUPITEM_H


#include "meta.h"
#include <QGraphicsItem>

class QFontMetricsF;

namespace Playlist
{
    /**
     * A QGraphicsItem based element for showing a group of tracks from a common album as one group
     * To take up less space and avoid having to show the same cover image over and over again in the
     * Playlist
     */
    class GraphicsAlbumGroupItem : public QGraphicsItem
    {

        public:
            GraphicsAlbumGroupItem();
            ~GraphicsAlbumGroupItem();
            void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
            QRectF boundingRect() const;
            qreal height() { return m_height; }
            //void play();

            void setAlbum( Meta::AlbumPtr album );
            void addTrack( Meta::TrackPtr track );


        protected:
           /* void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            void dropEvent( QGraphicsSceneDragDropEvent * event );
            void mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event );
            void mousePressEvent( QGraphicsSceneMouseEvent* event );
            void mouseMoveEvent( QGraphicsSceneMouseEvent* event );
            void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );*/
        
        private:
            void resize( Meta::TrackPtr track, int totalWidth );
            //int getRow() const { return int( ( mapToScene( 0.0, 0.0 ).y() ) / m_height ); }

            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;

            static QFontMetricsF* s_fm;


            //move to private class?
            qreal m_height;
            int m_lastWidth;
            Meta::TrackList m_tracks;
            Meta::AlbumPtr m_album;
    };

}
#endif

