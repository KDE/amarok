// Maintainer:  Max Howell (C) Copyright 2004
// Description: A fancy QSplitter type widget that also provides the browserBar and docking
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTSIDEBAR_H
#define PLAYLISTSIDEBAR_H

#include <qhbox.h>       //baseclass
//#include <qptrlist.h>    //stack allocated
//#include <qvaluelist.h>  //stack allocated
#include <qvaluevector.h>  //stack allocated
#include <qpushbutton.h> //baseclass

typedef QValueVector<QWidget*> PageList;
typedef QValueVector<QWidget*>::ConstIterator PageIterator;

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

    QVBox *container() const { return (QVBox*)m_playlist; }
    uint   position()  const { return m_pos; }

protected:
    bool eventFilter( QObject*, QEvent* );
    bool event( QEvent* );

private:
    QVBox  *m_playlist;
    QFrame *m_divider;
    uint    m_pos; //FIXME not required really, just use m_divider.left(), but it's only an int

public:
    void setFont( const QFont& );
    void addPage( QWidget*, const QString&, const QString& );
    QWidget *page( const QString& );

public slots:
    void showHidePage( int = -1 );
    void close() { showHidePage(); }
    void autoClosePages();
    void adjustSize();

private slots:
    void toggleOverlap( bool );

private:
    static const int DEFAULT_HEIGHT = 50;

    KMultiTabBar    *m_tabBar;
    QWidget         *m_pageHolder;
    QPushButton     *m_overlapButton;
    QSignalMapper   *m_mapper;
    PageList         m_pages;
    QWidget         *m_currentPage;
    KMultiTabBarTab *m_currentTab;

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

