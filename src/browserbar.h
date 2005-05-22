/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *   Copyright (C)       2005 Mark Kretschmann <markey@web.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BROWSERBAR_H
#define BROWSERBAR_H

#include "engineobserver.h" //baseclass
#include <qwidget.h>        //baseclass
#include <qvaluevector.h>   //stack allocated

typedef QValueVector<QWidget*> BrowserList;

class KURL;
class QSplitter;
class QToolBox;
class QVBox;


class BrowserBar : public QWidget, protected EngineObserver
{
    Q_OBJECT

public:
    BrowserBar( QWidget *parent );
   ~BrowserBar();

    QVBox *container() const { return m_playlistBox; }

    QWidget *browser( const QString& ) const;
    QWidget *browser( int index ) const { if( index < 0 ) index = 0; return m_browsers[index]; }
    QWidget *currentBrowser() const { return browser( m_currentIndex ); }

    void addBrowser( QWidget*, const QString&, const QString& );

protected:
    virtual void polish();

protected:
    virtual void engineStateChanged( Engine::State );

public slots:
    void showBrowser( const QString& name ) { showBrowser( indexForName( name ) ); }
    void showBrowser( int index ) { if( index != m_currentIndex ) showHideBrowser( index ); }
    void showHideBrowser( int );
    void closeCurrentBrowser() { showHideBrowser( m_currentIndex ); }

private:
    int indexForName( const QString& ) const;

    uint maxBrowserWidth() const { return width() / 2; }

    static const int DEFAULT_HEIGHT = 50;

    QSplitter     *m_splitter;
    QVBox         *m_playlistBox; ///parent to playlist, playlist filter and toolbar
    QToolBox      *m_toolBox;
    BrowserList    m_browsers;
    QVBox         *m_browserBox;  ///parent widget to the browsers
    int            m_currentIndex;
    int            m_lastIndex;
};

#endif
