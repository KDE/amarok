/***************************************************************************
                         playerwidget.h  -  description
                            -------------------
   begin                : Mit Nov 20 2002
   copyright            : (C) 2002 by Mark Kretschmann
   email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <qguardedptr.h> //stack allocated
#include <qlabel.h>      //stack allocated
#include <qpixmap.h>     //stack allocated
#include <qpushbutton.h> //baseclass
#include <qwidget.h>     //baseclass
#include "fht.h"         //stack allocated

class AmarokDcopHandler;
class AmarokSlider;
class AmarokSystray;
class ArtsConfigWidget;
class KActionCollection;
class KHelpMenu;
class KSystemTray;
class MetaBundle;
class PlayerApp;
class PlayerWidget;
class QBitmap;
class QButton;
class QHBox;
class QMouseEvent;
class QPaintEvent;
class QString;
class QStringList;


class NavButton : public QPushButton //no QOBJECT macro - why bother?
{
public: NavButton( QWidget*, const QString&, QObject*, const char* );
};

class IconButton : public QButton
{
Q_OBJECT
public:
    IconButton( QWidget*, const QString&/*, QObject*, const char*, bool=false*/ );
public slots:
    void setOn( bool b ) { QButton::setOn( b ); }
    void setOff()        { QButton::setOn( false ); }
private:
    void drawButton( QPainter* );
    const QPixmap m_up, m_down;
};


class PlayerWidget : public QWidget
{
        Q_OBJECT

    public:
        PlayerWidget( QWidget* =0, const char* =0 );
        ~PlayerWidget();

        friend class PlayerApp; //playerApp is fairly tied to this class

        void defaultScroll();
        void timeDisplay( int );
        void wheelEvent( QWheelEvent* ); //systray requires access
        void startDrag();

        // ATTRIBUTES ------
        KActionCollection *m_pActionCollection;

        static QString zeroPad( uint i ) { return ( i < 10 ) ? QString( "0%1" ).arg( i ) : QString::number( i ); }

    public slots:
        void createAnalyzer( int = 0 );
        void slotConfigShortcuts();       //TODO move to playerapp
        void slotConfigGlobalShortcuts(); //TODO move all generic stuff to playerapp

        void setScroll( const MetaBundle& );
        void drawScroll();

    signals:
        void configureDecoder();

    private:
        void setScroll( const QStringList& );
        void paintEvent( QPaintEvent *e );
        void mousePressEvent( QMouseEvent *e );
        void closeEvent( QCloseEvent *e );

        static const int SCROLL_RATE = 1;
        static const int VOLUME_MAX  = 100;

        // ATTRIBUTES ------

        QString m_rateString;

        AmarokDcopHandler *m_pDcopHandler; //TODO move to playerapp
        AmarokSystray     *m_pTray;
        QWidget           *m_pAnalyzer;
        KHelpMenu         *m_pHelpMenu;

        QPixmap m_scrollTextPixmap;
        QPixmap m_scrollBuffer;
        QPixmap m_timeBuffer;
        QPixmap m_plusPixmap;
        QPixmap m_minusPixmap;

        //widgets
        QFrame *m_pScrollFrame;
        QLabel *m_pTimeLabel;
        QLabel *m_pTimeSign;
        QLabel *m_pVolSign;
        QLabel *m_pDescription;
        QHBox  *m_pFrameButtons;
        IconButton   *m_pButtonEq;
        IconButton   *m_pButtonPl;
        AmarokSlider *m_pSlider;
        AmarokSlider *m_pVolSlider;
        QPushButton  *m_pButtonPlay;
        QPushButton  *m_pButtonPause;
};


//these two template functions are designed to reduce the amount of code in the PlayerWidget ctor
//and hopefully will be compiled such that the final binary is smaller too
template<class W> inline W*
wrapper( const QRect &r, QWidget *parent, const char *name = 0, QWidget::WFlags f = 0 )
{
    W *w = new W( parent, name, f );
    return placeWidget( w, r );
}

template<class W> inline W*
placeWidget( W *w, const QRect &r )
{
    w->setGeometry( r );
    return w;
}

#endif
