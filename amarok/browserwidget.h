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

class QWidget;
class QDropEvent;
class QDragMoveEvent;
class QFocusEvent;

class KDirLister;
class KURL;
class QString;

class PlayerApp;
extern PlayerApp *pApp;

class BrowserWidget : public KListView
{
    Q_OBJECT
        public:
        BrowserWidget( QWidget *parent=0, const char *name = 0 );
        ~BrowserWidget();

        void readDir( const KURL &url );

// ATTRIBUTES ------
        KDirLister *m_pDirLister;

    public slots:
        void slotCompleted();
        void slotReturnPressed( const QString& str );
        void slotHeaderClicked( int section );

    signals:
        void directoryChanged( const KURL & );
        void focusIn();
        void browserDrop();

    private:
        void contentsDropEvent( QDropEvent* e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void focusInEvent( QFocusEvent *e );
        QDragObject *dragObject();

// ATTRIBUTES ------
        int m_Count;
};
#endif
