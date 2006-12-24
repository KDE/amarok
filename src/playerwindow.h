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

#include <qpixmap.h>     //stack allocated
#include <qtoolbutton.h> //baseclass
#include <qvaluevector.h>//stack allocated
#include <qwidget.h>     //baseclass

#include "engineobserver.h" //baseclass

namespace Amarok { class PrettySlider; }
class KAction;
class MetaBundle;
class PlayerWidget;
class QBitmap;
class QButton;
class QHBox;
class QLabel;
class QString;
class QStringList;
class QTimerEvent;


class NavButton : public QToolButton //no QOBJECT macro - why bother?
{
public:
    NavButton( QWidget*, const QString&, KAction* );

protected:
    void timerEvent( QTimerEvent* );
    void drawButtonLabel( QPainter* );

    static const int GLOW_INTERVAL = 35;
    static const int NUMPIXMAPS    = 16;

    QPixmap m_pixmapOff;
    QPixmap m_pixmapDisabled;

    QValueVector<QPixmap> m_glowPixmaps;
    int m_glowIndex;
};


class IconButton : public QButton
{
    Q_OBJECT

public:
    IconButton( QWidget*, const QString&, const char *signal );
    IconButton( QWidget*, const QString&, QObject* receiver, const char *slot );

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

        /** Set modified Amarok palette */
        void setModifiedPalette();
        /** Call after some Amarok setting have changed */
        void applySettings();

        bool isMinimalView() { return m_minimalView; }
        void setMinimalView( bool enable );

        virtual void startDrag();

        /** Determines Amarok colours for current KDE scheme */
        static void determineAmarokColors();

    public slots:
        void createAnalyzer( int = 0 );
        void toggleView() { setMinimalView( !m_minimalView ); }


    protected:
        /** Observer reimpls **/
        void engineStateChanged( Engine::State state, Engine::State oldstate = Engine::Empty );
        void engineVolumeChanged( int percent );
        void engineNewMetaData( const MetaBundle &/*bundle*/, bool /*trackChanged*/ );
        void engineTrackPositionChanged( long /*position*/, bool /*userSeek*/ );
        void engineTrackLengthChanged( long length );

    signals:
        void playlistToggled( bool on );

    private slots:
        void drawScroll();
        void timeDisplay( int );
        void slotShowEqualizer( bool show );

    private:
        void setScroll( const QStringList& );

        virtual bool event( QEvent* );
        virtual bool eventFilter( QObject*, QEvent* );
        //virtual bool x11Event( XEvent* );
        virtual void paintEvent( QPaintEvent* );
        virtual void contextMenuEvent( QMouseEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual void mouseMoveEvent( QMouseEvent* );

        ///to make the code clearer to n00bies ;)
        QWidget *playlistWindow() { return parentWidget(); }

        static const int SCROLL_RATE = 1;
        static const int ANIM_TIMER  = 30;

        // ATTRIBUTES ------
        bool     m_minimalView;

        QTimer  *m_pAnimTimer;

        QPixmap m_scrollTextPixmap;
        QPixmap m_scrollBuffer;
        QPixmap m_timeBuffer;
        QPixmap m_plusPixmap;
        QPixmap m_minusPixmap;

        QPoint  m_startDragPos; //for drag behaviour

        //widgets
        QString      m_rateString;
        QWidget     *m_pAnalyzer;
        IconButton  *m_pButtonEq;
        IconButton  *m_pPlaylistButton;
        QLabel      *m_pTimeLabel;
        QLabel      *m_pTimeSign;

        QFrame  *m_pScrollFrame;
        QLabel  *m_pVolSign;
        QLabel  *m_pDescription;
        QHBox   *m_pFrameButtons;

        Amarok::PrettySlider *m_pSlider;
        Amarok::PrettySlider *m_pVolSlider;
        QToolButton    *m_pButtonPlay;
        QToolButton    *m_pButtonPause;

        QString m_currentURL;
};

#endif
