/***************************************************************************
                        browserwin.cpp  -  description
                           -------------------
  begin                : Fre Apr 24 2002
  copyright            : (C) 2002 by Frederik Holljen
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


#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <ktoolbar.h>
#include <kstatusbar.h>

#include <qevent.h>
#include <qlabel.h>

#include "engine/engineobserver.h"

namespace amaroK {
class ToggleLabel;

class StatusBar : public KStatusBar, public EngineObserver
{
    Q_OBJECT
public:
    StatusBar( QWidget *parent = 0, const char *name = 0 );
    virtual ~StatusBar();

    static StatusBar* self() { return m_self; }
    
protected: /* reimpl from engineobserver */
    virtual void engineStateChanged( EngineBase::EngineState state );
    virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
    virtual void engineTrackPositionChanged( long position );
    static QString zeroPad( uint i ) { return ( i < 10 ) ? QString( "0%1" ).arg( i ) : QString::number( i ); } // TODO: don't duplicate

private slots:
    void slotToggleTime();
    
private:
    void customEvent( QCustomEvent* e );
    
    static StatusBar* m_self;
    static const int ID_STATUS = 1;
    ToggleLabel *m_pTimeLabel;
};




class ToggleLabel : public QLabel
{
    Q_OBJECT
public:
    ToggleLabel( const QString &text, QWidget *parent = 0, const char *name = 0 );
    ~ToggleLabel();

    void setColorToggle( bool on );

protected:
    virtual void mouseDoubleClickEvent ( QMouseEvent * e );

public slots:
    virtual void setOn( bool );

signals:
    void toggled( bool state );

private:
    bool m_State;
    bool m_ColorToggle;
};

}
#endif // STATUSBAR_H
