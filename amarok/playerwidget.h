/***************************************************************************
                          playerwidget.h  -  description
                             -------------------
    begin                : Mit Nov 20 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                :
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

#include <qlabel.h>
#include <qwidget.h>
#include <qpixmap.h>
#include <qslider.h>

class QBitmap;
class QFrame;
class QMouseEvent;
class QMoveEvent;
class QPaintEvent;
class QPopupMenu;
class QPushButton;
class QString;
class QTimer;
class QTimerEvent;
class QToolButton;

class KActionCollection;

class VisWidget;
class ArtsConfigWidget;

class PlayerApp;
extern PlayerApp *pApp;

/**
 *@author mark
 */

// CLASS AmarokButton ------------------------------------------------------------

class AmarokButton : public QLabel
{
    Q_OBJECT

    public:
        AmarokButton( QWidget *parent, QString activePixmap, QString inactivePixmap, bool toggleButton );
        ~AmarokButton();

        void setOn( bool enable );
        bool isOn();
        
// ATTRIBUTES ------

    public slots:

    signals:
        void clicked();
        void toggled( bool on );

    private:
        void mousePressEvent( QMouseEvent *e );
        void mouseReleaseEvent( QMouseEvent *e );

// ATTRIBUTES ------
        QPixmap m_activePixmap, m_inactivePixmap;
        bool m_on;
        bool m_isToggleButton;
        bool m_clicked;
};



// CLASS AmarokSlider ------------------------------------------------------------

class AmarokSlider : public QSlider
{
    Q_OBJECT

    public:
        AmarokSlider( QWidget *parent );
        ~AmarokSlider();

        
// ATTRIBUTES ------

    public slots:

    signals:

    private:
        void mousePressEvent( QMouseEvent *e );
        
// ATTRIBUTES ------
};



// CLASS PlayerWidget ------------------------------------------------------------

class PlayerWidget : public QWidget
{
    Q_OBJECT

    public:
        PlayerWidget( QWidget *parent = 0, const char *name = 0 );
        ~PlayerWidget();

        void setScroll( QString text, QString bitrate, QString samplerate );
        void drawScroll();
        void timeDisplay( bool remaining, int hours, int minutes, int seconds );

// ATTRIBUTES ------
        KActionCollection *m_pActionCollection;
        
        QPopupMenu *m_pPopupMenu;
        VisWidget *m_pVis;
        QFrame *m_pFrame, *m_pFrameButtons;
        AmarokSlider *m_pSlider, *m_pSliderVol;
        QLabel *m_pTimeDisplayLabel;
        AmarokButton *m_pButtonPl, *m_pButtonEq, *m_pButtonLogo;
        QPushButton *m_pButtonPrev, *m_pButtonPlay, *m_pButtonPause, *m_pButtonStop, *m_pButtonNext;
        int m_IdRepeatTrack, m_IdRepeatPlaylist, m_IdConfPlayObject;
        ArtsConfigWidget *m_pPlayObjConfigWidget;

    public slots:
        void slotConfigShortcuts();
        void slotConfigGlobalShortcuts();
        void slotCopyClipboard();
        void slotConfigPlayObject();
        void slotConfigWidgetDestroyed();

    signals:

    private:
        void initScroll();
        void initTimeDisplay();
        void polish();

        void paintEvent( QPaintEvent * );
        void mouseReleaseEvent( QMouseEvent *e );
        void wheelEvent( QWheelEvent *e );
        void mousePressEvent( QMouseEvent *e );
        void closeEvent( QCloseEvent *e );
        void moveEvent( QMoveEvent *e );
        bool playObjectConfigurable();

// ATTRIBUTES ------
        QString m_bitrate, m_samplerate;
        QTimer *scrollTimer;
        QPixmap m_oldBgPixmap;
        QPixmap *m_pScrollPixmap, *m_pBgPixmap, *m_pComposePixmap;
        QBitmap *m_pScrollMask;
        QPixmap *m_pTimePixmap, *m_pTimeBgPixmap, *m_pTimeComposePixmap;
        int m_timeDisplayX, m_timeDisplayY, m_timeDisplayW;
        int m_pixmapWidth, m_pixmapHeight, m_scrollWidth;
        int m_sx, m_sy, m_sxAdd;
};
#endif
