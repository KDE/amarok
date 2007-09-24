/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTGRAPHICSVIEW_H
#define AMAROK_PLAYLISTGRAPHICSVIEW_H

#include "meta.h"
#include "PlaylistGraphicsAlbumGroupItem.h"

#include <QGraphicsView>

class GraphicsItem;
class QModelIndex;

namespace Playlist
{
    class GraphicsItem;
    class Model;
    class GraphicsView : public QGraphicsView
    {
        Q_OBJECT
        public:
            static GraphicsView *instance()
            {
                if( !s_instance )
                    s_instance = new GraphicsView();
                return s_instance;
            }
            void  setModel( Playlist::Model *model );

            const QList<GraphicsItem*> tracks() const { return m_tracks; }
            void  moveItem( Playlist::GraphicsItem *oldAbove, Playlist::GraphicsItem *newAbove );

        protected:
            virtual void contextMenuEvent( QContextMenuEvent *event );
            virtual void keyPressEvent( QKeyEvent *event );

        private slots:
            void modelReset();
            void rowsInserted( const QModelIndex & parent, int start, int end );
            void rowsRemoved( const QModelIndex & parent, int start, int end );
            void dataChanged( const QModelIndex & index );

            void playTrack();
            void removeSelection();
            void shuffleTracks( int startPosition, int stopPosition = -1 ); // -1: end of playlist
        
        private:
            GraphicsView( QWidget *parent = 0 );
            static GraphicsView  *s_instance;

            QList<GraphicsItem*>  m_tracks;
            QMap<Meta::AlbumPtr, Playlist::GraphicsAlbumGroupItem *> m_albumGroups;

            Model                *m_model;
    };

}

#endif
