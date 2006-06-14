/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXTBAR_H
#define AMAROK_CONTEXTBAR_H

#include <qwidget.h>        //baseclass
#include <qvaluevector.h>   //stack allocated

typedef QValueVector<QWidget*> BrowserList;

class MultiTabBar;
class MultiTabBarTab;
class KURL;
class QSignalMapper;
class QVBox;


class ContextBar : public QWidget
{
    Q_OBJECT

public:
    ContextBar( QWidget *parent );
   ~ContextBar();

    QVBox *container() const { return m_playlistBox; }

    QWidget *browser( const QString& ) const;
    QWidget *browser( int index ) const { if( index < 0 ) index = 0; return m_browsers[index]; }
    QWidget *currentBrowser() const { return m_currentIndex < 0 ? 0 : browser( m_currentIndex ); }

    int count() const { return m_browsers.count(); }
    int visibleCount() const;

    void addBrowser( QWidget*, const QString&, const QString&, const bool enabled = true );
    int restoreHeight();

    /// for internal use
    void mouseMovedOverSplitter( QMouseEvent* );

protected:
    virtual bool event( QEvent* );
    virtual void polish();

public slots:
    void showBrowser( const QString& name ) { showBrowser( indexForName( name ) ); }
    void showBrowser( int index ) { if( m_currentIndex != -1 && index != m_currentIndex ) showHideBrowser( index ); }
    void showHideBrowser( int );
    void showHideVisibleBrowser( int );
    void closeCurrentBrowser() { showHideBrowser( m_currentIndex ); }
    void setEnabled( int, const bool );
    void setEnabled( QWidget *widget , const bool enabled ) { setEnabled( indexForWidget( widget ), enabled ); }

signals:
    void browserActivated( QWidget* );

private:
    int indexForName( const QString& ) const;
    int indexForWidget( const QWidget* ) const;

    void adjustWidgetSizes();
    uint maxBrowserHeight() const { return uint( height() / 1.25 );  }


    uint           m_pos;         ///the y-axis position of m_divider
    QVBox         *m_playlistBox; ///parent to playlist, playlist filter and toolbar
    QWidget       *m_divider;     ///a qsplitter like widget
    MultiTabBar   *m_tabBar;
    BrowserList    m_browsers;
    QWidget       *m_browserBox;  ///parent widget to the browsers
    int            m_currentIndex;
    int            m_lastIndex;
    QSignalMapper *m_mapper;      ///maps tab clicks to browsers
};

#endif /*AMAROK_CONTEXTBAR_H*/
