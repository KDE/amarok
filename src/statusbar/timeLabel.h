/***************************************************************************
 *   Copyright (C) 2004-5 The amaroK Developers                            *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef AMAROK_TIMELABEL_H
#define AMAROK_TIMELABEL_H

#include <kpopupmenu.h>
#include <qlabel.h>

class TimeLabel : public QLabel
{
public:
    TimeLabel( QWidget *parent ) : QLabel( " 0:00:00 ", parent )
    {
        setFont( KGlobalSettings::fixedFont() );
        setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    }

    virtual void mousePressEvent( QMouseEvent* e )
    {
        if ( e->button() == Qt::LeftButton )
            AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );

        if ( e->button() == Qt::RightButton ) {
            enum { NORMAL, REMAIN, LENGTH };

            KPopupMenu menu;
            menu.setCheckable( true );
            menu.insertTitle( i18n( "Time Display" ) );
            menu.insertItem( i18n( "Normal" ), NORMAL );
            menu.insertItem( i18n( "Remaining" ), REMAIN );
            menu.insertItem( i18n( "Length" ), LENGTH );
            menu.setItemChecked( NORMAL, !AmarokConfig::timeDisplayRemaining() );
            menu.setItemChecked( REMAIN, AmarokConfig::timeDisplayRemaining() );
            menu.setItemEnabled( LENGTH, false );

            switch ( menu.exec( e->globalPos() ) ) {
            case NORMAL:
                AmarokConfig::setTimeDisplayRemaining( false );
                break;
            case REMAIN:
                AmarokConfig::setTimeDisplayRemaining( true );
                break;
            case LENGTH:
                break;
            }
        }
    }
};
#endif
