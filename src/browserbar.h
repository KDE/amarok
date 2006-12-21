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

#include "amarok_export.h"  //LIBAMAROK_EXPORT
#include "engineobserver.h" //baseclass

#include <qwidget.h>        //baseclass
#include <qvaluevector.h>   //stack allocated
#include <qmap.h>           //stack allocated
#include <qpushbutton.h>

typedef QValueVector<QWidget*> BrowserList;
typedef QMap<QString,int> BrowserIdMap;

class MultiTabBar;
class MultiTabBarTab;
class DropProxyTarget;
class KURL;
class QSignalMapper;
class QVBox;


class BrowserBar : public QWidget, public EngineObserver
{
    Q_OBJECT

public:
    BrowserBar( QWidget *parent );
   ~BrowserBar();

    LIBAMAROK_EXPORT static BrowserBar* instance() { return s_instance; }
    QVBox *container() const { return m_playlistBox; }
    QVBox *browserBox() const { return m_browserBox; }

    QWidget *browser( const QString& ) const;
    QWidget *browser( int index ) const { if( index < 0 ) index = 0; return m_browsers[index]; }
    QWidget *currentBrowser() const { return m_currentIndex < 0 ? 0 : browser( m_currentIndex ); }

    int count() const { return m_browsers.count(); }
    int visibleCount() const;

    void addBrowser( const QString &identifier, QWidget*, const QString&, const QString& );
    void removeMediaBrowser( QWidget *widget );
    int indexForName( const QString& ) const;
    int restoreWidth();

    /// for internal use
    void mouseMovedOverSplitter( QMouseEvent* );
    void makeDropProxy( const QString &browserName, DropProxyTarget *finalTarget );

protected:
    virtual bool event( QEvent* );
    virtual void polish();

protected:
    virtual void engineStateChanged( Engine::State, Engine::State = Engine::Empty );

public slots:
    void showBrowser( const QString& name ) { showBrowser( indexForName( name ) ); }
    void showBrowser( int index ) { if( index != m_currentIndex ) showHideBrowser( index ); }
    void showHideBrowser( int );
    void showHideVisibleBrowser( int );
    void closeCurrentBrowser() { showHideBrowser( m_currentIndex ); }
    void showBrowserSelectionMenu();

signals:
    void browserActivated( int );

private:
    void adjustWidgetSizes();
    uint maxBrowserWidth() const { return width() * 2 / 3; }

    static const int DEFAULT_HEIGHT = 50;

    LIBAMAROK_EXPORT static BrowserBar    *s_instance;
    uint           m_pos;         ///the x-axis position of m_divider
    QVBox         *m_playlistBox; ///parent to playlist, playlist filter and toolbar
    QWidget       *m_divider;     ///a qsplitter like widget
    MultiTabBar   *m_tabBar;
    BrowserList    m_browsers;
    BrowserIdMap   m_browserIds;
    QVBox         *m_browserBox;  ///parent widget to the browsers
    int            m_currentIndex;
    int            m_lastIndex;
    QSignalMapper *m_mapper;      ///maps tab clicks to browsers

    QPushButton   *m_tabManagementButton;


    
};

#endif
