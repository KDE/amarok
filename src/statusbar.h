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

#include <kstatusbar.h>
#include <qlabel.h>

#include "engineobserver.h"

class KAction;
class KProgress;
class KToggleAction;
class PlaylistSlider;
class QCustomEvent;
class QTimer;

namespace amaroK {

class ToggleLabel;

class StatusBar : public KStatusBar, public EngineObserver
{
    Q_OBJECT
public:
    StatusBar( QWidget *parent = 0, const char *name = 0 );
    virtual ~StatusBar();

    static StatusBar* self() { return m_self; }

public slots:
    /** update total song count */
    void slotItemCountChanged(int newCount);

protected: /* reimpl from engineobserver */
    virtual void engineStateChanged( EngineBase::EngineState state );
    virtual void engineTrackPositionChanged( long position );
    virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

private slots:
    void sliderPressed();
    void sliderReleased();
    void sliderMoved( int value );
    void slotPauseTimer();

private:
    void customEvent( QCustomEvent* e );
    void drawTimeDisplay( long position );

    static StatusBar* m_self;

    QLabel         *m_pTimeLabel;
    QLabel         *m_pTitle;
    QLabel         *m_pTotal;
    KProgress      *m_pProgress;
    PlaylistSlider *m_pSlider;
    bool            m_sliderPressed;
    QTimer         *m_pPauseTimer;
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

} //namespace amaroK


#endif //AMAROK_STATUSBAR_H
