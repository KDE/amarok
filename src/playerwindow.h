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

#include <qlabel.h>      //stack allocated
#include <qpixmap.h>     //stack allocated
#include <qptrlist.h>    //stack allocated
#include <qpushbutton.h> //baseclass
#include <qwidget.h>     //baseclass

#include <vector>

#include "fht.h"         //stack allocated
#include "engineobserver.h"

namespace amaroK { class Slider; }
class KAction;
class MetaBundle;
class PlayerWidget;
class QBitmap;
class QButton;
class QHBox;
class QString;
class QStringList;
class QTimerEvent;


class NavButton : public QPushButton //no QOBJECT macro - why bother?
{
public:
    NavButton( QWidget*, const QString&, KAction* );

protected:
    void timerEvent( QTimerEvent* );
    void drawButtonLabel( QPainter* );

    static const int GLOW_INTERVAL = 35;
    static const int NUMPIXMAPS = 16;

    QPixmap m_pixmapOff;
    QPixmap m_pixmapDisabled;

    QPtrList<QPixmap> m_glowPixmaps;
    int m_glowIndex;
};


class IconButton : public QButton
{
    Q_OBJECT

public:
    IconButton( QWidget*, const QString&, const char *signal );

public slots:
    void setOn( bool b ) { QButton::setOn( b ); }
    void setOff()        { QButton::setOn( false ); }

private:
    void drawButton( QPainter* );

    const QPixmap m_up;
    const QPixmap m_down;
};


class PlayerWidget : public QWidget, public EngineObserver
{
        Q_OBJECT

    public:
        PlayerWidget( QWidget* = 0, const char* = 0, bool enablePlaylist = false );
        ~PlayerWidget();

        virtual void startDrag();

        static void determineAmarokColors();

    public slots:
        void createAnalyzer( int = 0 );
        void setEffectsWindowShown( bool );

    protected:
    /** Observer reimpls **/
        void engineStateChanged( Engine::State state );
        void engineVolumeChanged( int percent );
        void engineNewMetaData( const MetaBundle &/*bundle*/, bool /*trackChanged*/ );
        void engineTrackPositionChanged( long /*position*/ );

    signals:
        void playlistToggled( bool on );
        void effectsWindowToggled( bool );

    private slots:
        void drawScroll();
        void timeDisplay( int );

    private:
        void setScroll( const QStringList& );

        virtual bool event( QEvent* );
        virtual bool eventFilter( QObject*, QEvent* );
        virtual void paintEvent( QPaintEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual void mouseMoveEvent( QMouseEvent* );


        static const int SCROLL_RATE = 1;
        static const int ANIM_TIMER  = 30;

        // ATTRIBUTES ------

        QTimer  *m_pAnimTimer;
        QString  m_rateString;

        QPixmap m_scrollTextPixmap;
        QPixmap m_scrollBuffer;
        QPixmap m_timeBuffer;
        QPixmap m_plusPixmap;
        QPixmap m_minusPixmap;

        QPoint  m_startDragPos; //for drag behaviour

        //widgets
        QWidget *m_pAnalyzer;
        QFrame  *m_pScrollFrame;
        QLabel  *m_pTimeLabel;
        QLabel  *m_pTimeSign;
        QLabel  *m_pVolSign;
        QLabel  *m_pDescription;
        QHBox   *m_pFrameButtons;
        IconButton     *m_pButtonFx;
        IconButton     *m_pPlaylistButton;
        amaroK::Slider *m_pSlider;
        amaroK::Slider *m_pVolSlider;
        QPushButton    *m_pButtonPlay;
        QPushButton    *m_pButtonPause;
};

#endif
