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

#include "amarok_export.h"  //AMAROK_EXPORT
#include "engineobserver.h" //baseclass

#include <QWidget>        //baseclass
#include <Q3ValueVector>   //stack allocated
#include <QMap>           //stack allocated
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>

typedef Q3ValueVector<QWidget*> BrowserList;
typedef QMap<QString,int> BrowserIdMap;

class MultiTabBar;
class DropProxyTarget;
class QSignalMapper;
class KVBox;


class BrowserBar : public QWidget, public EngineObserver
{
    Q_OBJECT

public:
    BrowserBar( QWidget *parent );
   ~BrowserBar();

    AMAROK_EXPORT static BrowserBar* instance() { return s_instance; }
    KVBox *container() const { return m_playlistBox; }
    KVBox *browserBox() const { return m_browserBox; }

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

    AMAROK_EXPORT static BrowserBar    *s_instance;
    uint           m_pos;         ///the x-axis position of m_divider
    KVBox         *m_playlistBox; ///parent to playlist, playlist filter and toolbar
    QWidget       *m_divider;     ///a qsplitter like widget
    MultiTabBar   *m_tabBar;
    BrowserList    m_browsers;
    BrowserIdMap   m_browserIds;
    KVBox         *m_browserBox;  ///parent widget to the browsers
    int            m_currentIndex;
    int            m_lastIndex;
    QSignalMapper *m_mapper;      ///maps tab clicks to browsers

    QPushButton   *m_tabManagementButton;


    
};

#endif
