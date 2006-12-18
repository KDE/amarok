/***************************************************************************
 *   Copyright (C) 2004-5 The Amarok Developers                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_TIMELABEL_H
#define AMAROK_TIMELABEL_H

#include <qlabel.h>

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

        Amarok::StatusBar::instance()->drawTimeDisplay( EngineController::engine()->position() );
    }
};

#endif /*AMAROK_TIMELABEL_H*/
