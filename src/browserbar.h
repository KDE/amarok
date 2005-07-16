/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
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

class MultiTabBar;
class MultiTabBarTab;
class KURL;
class QSignalMapper;
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

    /// for internal use
    void mouseMovedOverSplitter( QMouseEvent* );

protected:
    virtual bool eventFilter( QObject*, QEvent* );
    virtual bool event( QEvent* );
    virtual void polish();
    virtual void timerEvent( QTimerEvent* );

protected:
    virtual void engineStateChanged( Engine::State, Engine::State = Engine::Empty );

public slots:
    void showBrowser( const QString& name ) { showBrowser( indexForName( name ) ); }
    void showBrowser( int index ) { if( index != m_currentIndex ) showHideBrowser( index ); }
    void showHideBrowser( int );
    void closeCurrentBrowser() { showHideBrowser( m_currentIndex ); }

private:
    int indexForName( const QString& ) const;

    void adjustWidgetSizes();
    uint maxBrowserWidth() const { return width() / 2; }

    static const int DEFAULT_HEIGHT = 50;

    uint           m_pos;         ///the x-axis position of m_divider
    QVBox         *m_playlistBox; ///parent to playlist, playlist filter and toolbar
    QWidget       *m_divider;     ///a qsplitter like widget
    MultiTabBar   *m_tabBar;
    BrowserList    m_browsers;
    QWidget       *m_browserBox;  ///parent widget to the browsers
    int            m_currentIndex;
    int            m_lastIndex;
    QSignalMapper *m_mapper;      ///maps tab clicks to browsers
};

#endif
