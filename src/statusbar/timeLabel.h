/*
    Copyright (c) 2005 Max Howell <max.howell@methylblue.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AMAROK_TIMELABEL_H
#define AMAROK_TIMELABEL_H

#include "ActionClasses.h"
#include "EngineController.h"
#include "ProgressSlider.h"

#include <QLabel>
#include <kglobalsettings.h>
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
        if( AmarokConfig::leftTimeDisplayEnabled() )
        {
            AmarokConfig::setLeftTimeDisplayEnabled( false );
            AmarokConfig::setLeftTimeDisplayRemaining( true );
        }
        else if( AmarokConfig::leftTimeDisplayRemaining() )
        {
            AmarokConfig::setLeftTimeDisplayRemaining( false );
        }
        else
        {
            AmarokConfig::setLeftTimeDisplayEnabled( true );
        }

        ProgressWidget::instance()->drawTimeDisplay( The::engineController()->trackPosition() );
    }
};

#endif /*AMAROK_TIMELABEL_H*/
