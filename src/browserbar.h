// Maintainer: Max Howell (C) Copyright 2004
// Copyright:  See COPYING file that comes with this distribution
//

#ifndef PLAYLISTSIDEBAR_H
#define PLAYLISTSIDEBAR_H

#include <qhbox.h>        //baseclass
#include <qpushbutton.h>  //baseclass
#include <qvaluevector.h> //stack allocated

typedef QValueVector<QWidget*> BrowserList;
typedef QValueVector<QWidget*>::ConstIterator BrowserIterator;

class KMultiTabBar;
class KMultiTabBarTab;
class QEvent;
class QObject;
class QObjectList;
class QPixmap;
class QPushButton;
class QResizeEvent;
class QSignalMapper;
class QVBox;

static const char* const not_close_xpm[]={
"5 5 2 1",
"# c black",
". c None",
"#####",
"#...#",
"#...#",
"#...#",
"#####"};



class BrowserBar : public QWidget
{
    Q_OBJECT

public:
    BrowserBar( QWidget *parent );
    ~BrowserBar();

    QVBox   *container() const { return (QVBox*)m_playlist; }
    QWidget *browser( const QCString& ) const;
    uint     position() const { return m_pos; }

    void     setFont( const QFont& );
    void     addBrowser( QWidget*, const QString&, const QString& );

protected:
    bool eventFilter( QObject*, QEvent* );
    bool event( QEvent* );

public slots:
    void showHideBrowser( int = -1 );
    void close() { showHideBrowser(); }
    void autoCloseBrowsers();

private slots:
    void toggleOverlap( bool );

private:
    void adjustWidgetSizes();

    static const int DEFAULT_HEIGHT = 50;

    uint             m_pos; //the x-axis position of m_divider
    QVBox           *m_playlist; //not the playlist, but parent to the playlist and searchBar
    QWidget         *m_divider; //a qsplitter like widget
    KMultiTabBar    *m_tabBar;
    BrowserList      m_browsers; //the browsers are stored in this qvaluevector
    QWidget         *m_browserHolder; //parent widget to the browsers
    QWidget         *m_currentBrowser; //currently displayed page, may be 0
    KMultiTabBarTab *m_currentTab;  //currently open tab, may be 0
    QPushButton     *m_overlapButton;

    QSignalMapper   *m_mapper; //maps tab clicks to browsers


    class TinyButton : public QPushButton
    {
    public:
        TinyButton( QWidget*, const QPixmap&, const QString& );

    protected:
        virtual void drawButton( QPainter* );
        virtual void enterEvent( QEvent* ) { m_mouseOver = true; repaint( false ); }
        virtual void leaveEvent( QEvent* ) { m_mouseOver = false; repaint( false ); }

    private:
        bool m_mouseOver;
    };
};

#endif

