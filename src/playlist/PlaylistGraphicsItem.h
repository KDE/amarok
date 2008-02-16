/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#ifndef AMAROK_PLAYLISTGRAPHICSITEM_H
#define AMAROK_PLAYLISTGRAPHICSITEM_H


#include "Meta.h"
#include <QGraphicsItem>
#include <QSvgRenderer>

class QFontMetricsF;

namespace Playlist
{
    /**
     * A lazy-loading QGraphicsItem to display one track in the playlist.
     * If a user drags 20000 tracks into the playlist, 20000 GraphicsItem's
     * will be created. However only the tracks that are visible will query
     * the model for their information, the rest will take up very little memory
     * and really aren't associated with a particular track yet.
     * On a paint operation the GraphicsItem will be "active" by creating an ActiveItems.
     * Do not add any data members to GraphicsItem, you should be able to add them to
     * ActiveItems instead.
     */
    class GraphicsItem : public QGraphicsItem
    {
        class ActiveItems;

        public:
            GraphicsItem();
            ~GraphicsItem();
            ///Be sure to read ::paint rules in-method
            void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
            QRectF boundingRect() const;
            void setupItem();
            void refresh();
            void play();

            void setRow( int row );
            const QRectF imageLocation() const { return QRectF( MARGIN, MARGIN, ALBUM_WIDTH, ALBUM_WIDTH ); }

            const bool hasImage() const;
            void showImage() const;
            void fetchImage();
            void unsetImage();
            void dataChanged();

            void editTrackInformation();

            const int groupMode() const { return m_groupMode; }
       
        protected:
            void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            void dropEvent( QGraphicsSceneDragDropEvent * event );
            void mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event );
            void mousePressEvent( QGraphicsSceneMouseEvent* event );
            void mouseMoveEvent( QGraphicsSceneMouseEvent* event );
            void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
            void hoverEnterEvent ( QGraphicsSceneHoverEvent * event ); 
        
        private:
            void init( Meta::TrackPtr track );
            void resize( Meta::TrackPtr track, int totalWidth );
            QString findArtistForCurrentAlbum() const;

            
            void paintSingleTrack ( QPainter* painter, const QStyleOptionGraphicsItem* option, bool active );
            void paintHead ( QPainter* painter, const QStyleOptionGraphicsItem* option, bool active );
            void paintCollapsedHead ( QPainter* painter, const QStyleOptionGraphicsItem* option, bool active );
            void paintBody( QPainter* painter, const QStyleOptionGraphicsItem* option, bool active, bool alternate );
            void paintTail( QPainter* painter, const QStyleOptionGraphicsItem* option, bool active, bool alternate  );
            void paintCollapsed( );

            QPixmap getCachedSvg( QString name, int width, int height );
            void handleActiveOverlay( QRectF rect, bool active );

            ActiveItems* m_items;
            qreal m_height;
            int m_groupMode;
            int m_currentRow;
            bool m_groupModeChanged;
            bool m_collapsible;
            bool m_dataChanged;

            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;
            static QFontMetricsF* s_fm;
            static QSvgRenderer * s_svgRenderer;
    };

}
#endif

