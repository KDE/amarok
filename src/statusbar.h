/***************************************************************************
 statusbar.h          : amaroK browserwin statusbar
 copyright            : (C) 2004 by Frederik Holljen
 email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_STATUSBAR_H
#define AMAROK_STATUSBAR_H

#include "engineobserver.h" //baseclass
#include <kstatusbar.h>     //baseclass
#include <qevent.h>         //baseclass
#include <qlabel.h>         //baseclass

class KAction;
class KProgress;
class KToggleAction;
class QCustomEvent;
class QTimer;


namespace amaroK {

class Slider;
class ToggleLabel;

class StatusBar : public KStatusBar, public EngineObserver
{
    Q_OBJECT
public:
    StatusBar( QWidget *parent = 0, const char *name = 0 );
    ~StatusBar();

    static StatusBar* instance() { return s_instance; }

    static void startProgress();
    static void showProgress( uint );
    static void stopProgress();

public slots:
    /** update total song count */
    void slotItemCountChanged(int newCount);
    void message( const QString& message ); //reimpl. from QStatusBar
    void message( const QString&, int ms ); //reimpl. from QStatusBar
    void clear();
    void engineMessage( const QString &s ) { message( s, 3000 ); } //NOTE leave inlined!

protected: /* reimpl from engineobserver */
    virtual void engineStateChanged( Engine::State state );
    virtual void engineTrackPositionChanged( long position );
    virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

private slots:
    void slotPauseTimer();
    void drawTimeDisplay( int position );
    void stopPlaylistLoader();

private:
    virtual void customEvent( QCustomEvent* e );

    static StatusBar* s_instance;

    QLabel         *m_pTimeLabel;
    QLabel         *m_pTitle;
    QLabel         *m_pTotal;
    KProgress      *m_pProgress;
    QWidget        *m_pProgressBox;
    amaroK::Slider *m_pSlider;
    bool            m_sliderPressed;
    QTimer         *m_pPauseTimer;
    QString         m_oldMessage;
};


class ToggleLabel : public QLabel
{
    Q_OBJECT
public:
    ToggleLabel( const QString&, KStatusBar* const, KToggleAction* const );

protected:
    virtual void mouseDoubleClickEvent ( QMouseEvent* );

public slots:
    void setChecked( bool );

private:
    bool     m_state;
    KAction *m_action;
};


class ProgressEvent : public QCustomEvent
{
public:
    ProgressEvent( int progress ) : QCustomEvent( 5000 ), m_value( progress ) {}

    int progress() { return m_value; }

    static const int Type = 5000;

private:
    int m_value;
};
} //namespace amaroK


#endif //AMAROK_STATUSBAR_H
