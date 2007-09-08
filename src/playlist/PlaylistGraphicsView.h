/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTGRAPHICSVIEW_H
#define AMAROK_PLAYLISTGRAPHICSVIEW_H

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
            GraphicsView( QWidget* parent, Model* model );

            const QList<GraphicsItem*> tracks() const { return m_tracks; }
            void shuffleTracks( int startPosition );
            void shuffleTracks( Playlist::GraphicsItem *startPosition );

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
        
        private:
            QList<GraphicsItem*>  m_tracks;
            Model                *m_model;
    };

}

#endif
