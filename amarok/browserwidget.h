/***************************************************************************
                          browserwidget.h  -  description
                             -------------------
    begin                : Don Nov 14 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include <klistview.h>

class PlayerApp;
extern PlayerApp *pApp;

class QWidget;
class QDropEvent;
class QDragMoveEvent;
class QKeyEvent;
class QListView;
class QListViewItem;
class KDirLister;


//TODO move #includes and impl to browserwidget.cpp
#include <kurl.h>
#include <qstring.h>
class FileBrowserItem : public KListViewItem
{
    public:
        FileBrowserItem( QListView *lv )
           : KListViewItem( lv, 0, QString( ".." ) )
           , m_isDir( true )
        {}
        FileBrowserItem( QListView *lv, QListViewItem *lvi, const KURL &u, bool b = false )
           : KListViewItem( lv, lvi, u.fileName() )
           , m_url( u )
           , m_isDir( b )
        {}

        const KURL &url()   const { return m_url; }
        const bool &isDir() const { return m_isDir; }        

    private:
        const KURL m_url;
        const bool m_isDir;
};


class BrowserWidget : public KListView
{
    Q_OBJECT
        public:
        BrowserWidget( QWidget *parent=0, const char *name = 0 );
        ~BrowserWidget();

        void readDir( const KURL &url );

// ATTRIBUTES ------
        KDirLister *m_pDirLister;
        QString cachedPath;

    public slots:
        void slotCompleted();
        void slotReturnPressed( const QString& str );
        void slotHeaderClicked( int section );

    signals:
        void directoryChanged( const KURL & );
        void browserDrop();

    private:
        void contentsDropEvent( QDropEvent* e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void keyPressEvent( QKeyEvent *e );
        QDragObject *dragObject();
};
#endif
