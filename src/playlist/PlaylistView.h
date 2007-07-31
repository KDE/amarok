/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTVIEW_H
#define AMAROK_PLAYLISTVIEW_H

#include <QBasicTimer>
#include <QListView>
#include <QPersistentModelIndex>
#include <QHash>

class QAbstractItemModel;
class QKeyEvent;
class QMouseEvent;

namespace PlaylistNS {
/**
 * The view of the playlist, used to send user interaction signals back to the model.
 */
    class View : public QListView
    {
        Q_OBJECT
        public:
            View( QWidget* parent );
            ~View();
            virtual void setModel( QAbstractItemModel * model );
        protected:
            virtual void keyPressEvent( QKeyEvent* event );
            virtual void mouseDoubleClickEvent(QMouseEvent *event);
    };
/**
 * Animate the delegates GraphicsView's. Based from the Advanced Item View talk at Trolltech
 * Developer Days 2006.
 * Singleton, deleted by ~View. 
 */    
    class Animator : public QObject
    {
        Q_OBJECT
        public:
            Animator( PlaylistNS::View* view ) : QObject(), m_view( view ) { s_instance = this; }
            static Animator* instance() { return s_instance; }
            void startAnimation( const QModelIndex& animatedRow, QGraphicsScene* animatedScene );
            void stopAnimation( const QModelIndex& dullRow );
        private slots:
            void paintRow();

        private:
            QBasicTimer m_timer;
            QHash<QObject*, QPersistentModelIndex*> m_modelHash;
            PlaylistNS::View* m_view;

            static Animator* s_instance;
    };
    
}
#endif
