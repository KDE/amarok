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


#ifndef AMAROK_PLAYLISTGRAPHICSVIEW_H
#define AMAROK_PLAYLISTGRAPHICSVIEW_H

#include "Meta.h"
#include "UndoCommands.h"

#include <QGraphicsItemAnimation>
#include <QGraphicsView>
#include <QTimeLine>

class GraphicsItem;
class QModelIndex;

namespace Playlist {
    class GraphicsView;
}

namespace The {
    Playlist::GraphicsView*  playlistView();
}


namespace Playlist
{
    class GraphicsItem;
    class Model;
    class GraphicsView : public QGraphicsView
    {
        Q_OBJECT

        friend Playlist::GraphicsView* The::playlistView();

        friend class Playlist::MoveTracksCmd;

        public:
            void  setModel( Playlist::Model *model );

            const QList<GraphicsItem*> tracks() const { return m_tracks; }
            void  moveItem( Playlist::GraphicsItem *oldAbove, Playlist::GraphicsItem *newAbove );

        public slots:
            virtual void dropEvent( QDropEvent *event );

        protected:
            virtual void contextMenuEvent( QContextMenuEvent *event );
            virtual void dragEnterEvent( QDragEnterEvent *event );
            virtual void dragMoveEvent( QDragMoveEvent *event );
            virtual void dragLeaveEvent( QDragLeaveEvent *event );
            virtual void keyPressEvent( QKeyEvent *event );
            virtual void paletteChange( const QPalette & oldPalette );

            //virtual void drawBackground ( QPainter * painter, const QRectF & rect );
            

        private slots:
            void modelReset();
            void rowsInserted( const QModelIndex & parent, int start, int end );
            void rowsRemoved( const QModelIndex & parent, int start, int end );
            void dataChanged( const QModelIndex & index );
            void moveViewItem( int row, int to );
            void groupingChanged();
            void rowsChanged( int start );

            void playTrack();
            void removeSelection();
            void shuffleTracks( int startPosition, int stopPosition = -1, bool animate = true ); // -1: end of playlist
            void showItemImage();
            void fetchItemImage();
            void unsetItemImage();
            void animationComplete();
            void editTrackInformation();
        
        private:
            GraphicsView( QWidget *parent = 0 );
            static GraphicsView  *s_instance;

            QList<GraphicsItem*>  m_tracks;
            Model                *m_model;
            Playlist::GraphicsItem *m_contextMenuItem;

            QMultiHash< QTimeLine*, QGraphicsItemAnimation* > m_animatorsByTimeline;
            QHash< QGraphicsItem*, QGraphicsItemAnimation* > m_animatorsByItem;
    };

}

#endif
