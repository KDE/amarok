/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "AmarokOSDScript.h"

#include "App.h"
#include "amarokconfig.h"
#include "widgets/Osd.h"

#include <QJSEngine>

using namespace AmarokScript;

AmarokOSDScript::AmarokOSDScript( QJSEngine *engine )
    : QObject( engine )
{
    QJSValue scriptObject = engine->newQObject( this) ;
    QJSValue windowObject = engine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Window") );
    windowObject.setProperty( QStringLiteral("OSD"), scriptObject );
}

void
AmarokOSDScript::showCurrentTrack()
{
    Amarok::OSD::instance()->forceToggleOSD();
}

void
AmarokOSDScript::show()
{
    Amarok::OSD::instance()->show();
}

void
AmarokOSDScript::setDuration( int ms )
{
    Amarok::OSD::instance()->setDuration( ms );
}

void
AmarokOSDScript::setTextColor( const QColor &color )
{
    Amarok::OSD::instance()->setTextColor( color );
}

void
AmarokOSDScript::setOffset( int y )
{
    Amarok::OSD::instance()->setYOffset( y );
}

void
AmarokOSDScript::setImage( const QPixmap &image )
{
    Amarok::OSD::instance()->setImage( image );
}

void
AmarokOSDScript::setScreen( int screen )
{
    Amarok::OSD::instance()->setScreen( screen );
}

void
AmarokOSDScript::setText( const QString &text )
{
    Amarok::OSD::instance()->setText( text );
}

void
AmarokOSDScript::setRating( const short rating )
{
    Amarok::OSD::instance()->setRating( rating );
}

void
AmarokOSDScript::setOsdEnabled( bool enable )
{
    Amarok::OSD::instance()->setEnabled( enable );
    AmarokConfig::setOsdEnabled( enable );
}

bool
AmarokOSDScript::osdEnabled()
{
    return AmarokConfig::osdEnabled();
}
