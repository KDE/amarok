//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTSIDEBAR_H
#define PLAYLISTSIDEBAR_H

//In order to remember the size of the tabs when using a QSplitter it is
//necessary to override sizeHint(). Hence this class.
//Later it seemed convenient to move management of the widgets (pages)
//here too, so I did that too.

#include <qhbox.h>       //baseclass
#include <qptrlist.h>    //stack allocated
#include <qpushbutton.h> //baseclass

class KMultiTabBar;
class QSignalMapper;
class QResizeEvent;
class QPushButton;

static const char* const not_close_xpm[]={
"5 5 2 1",
"# c black",
". c None",
"#####",
"#...#",
"#...#",
"#...#",
"#####"};


class PlaylistSideBar : public QHBox
{
Q_OBJECT

public:
    PlaylistSideBar( QWidget *parent );
    ~PlaylistSideBar();

    void setFont( const QFont& );
    void addPage( QWidget*, const QString&, const QString& );
    QWidget *page( const QString& );
    virtual QSize sizeHint() const;

public slots:
    void showHidePage( int );
    void close();
    void autoClosePages();

private:
    static const int DefaultHeight = 50;

    KMultiTabBar     *m_multiTabBar;
    QWidget          *m_pageHolder;
    QPushButton      *m_stayButton;
    QSignalMapper    *m_mapper;
    QPtrList<QWidget> m_pages;

    virtual void  resizeEvent( QResizeEvent * );


    // CLASS DockButton =================

    class TinyButton : public QPushButton
    {
    public:
        TinyButton( QWidget * = 0 );

    protected:
        virtual void drawButton( QPainter * );
        virtual void enterEvent( QEvent * );
        virtual void leaveEvent( QEvent * );

    private:
        bool m_mouseOver;
    };
};

#endif

