/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TIMELABEL_H
#define AMAROK_TIMELABEL_H

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "EngineController.h"
#include "ProgressWidget.h"

#include <QLabel>
#include <kglobalsettings.h>
#include <QFontMetrics>
#include <QMouseEvent>


class TimeLabel : public QLabel
{
public:
    TimeLabel( QWidget *parent ) : QLabel( " 0:00:00 ", parent )
    {
        setFont( KGlobalSettings::fixedFont() );
        setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    }

    virtual void mousePressEvent( QMouseEvent * )
    {
        AmarokConfig::setLeftTimeDisplayRemaining( !AmarokConfig::leftTimeDisplayRemaining() );
        ProgressWidget * progressWidget = dynamic_cast<ProgressWidget *>( parentWidget() );
        if( progressWidget )
            progressWidget->drawTimeDisplay( The::engineController()->trackPositionMs() );
    }

    virtual QSize sizeHint() const
    {
        return fontMetrics().boundingRect( KGlobal::locale()->negativeSign() + KGlobal::locale()->formatTime( QTime( 0, 0, 0 ), true, true ) ).size();
    }

    void setShowTime( bool showTime )
    {
        m_showTime = showTime;
        if( !showTime )
        {
            QLabel::setText( "" );
        }
    }

    bool showTime() const
    {
        return m_showTime;
    }

    void setText( const QString &text )
    {
        if( m_showTime )
            QLabel::setText( text );
    }

private:
    bool m_showTime;

};

#endif /*AMAROK_TIMELABEL_H*/
