/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "GrowlInterface.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h" // for secToPrettyTime
#include "SvgHandler.h"
#include "TrayIcon.h"

GrowlInterface::GrowlInterface( QString appName ) :
                m_appName( appName )
{
    EngineController *engine = The::engineController();

    connect( engine, &EngineController::trackChanged,
             this, &GrowlInterface::show );
}

void
GrowlInterface::show( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    QString text;
    if( !track || track->playableUrl().isEmpty() )
        text = i18n( "No track playing" );
    else
    {
        text = track->prettyName();
        if( track->artist() && !track->artist()->prettyName().isEmpty() )
            text = track->artist()->prettyName() + " - " + text;
        if( track->album() && !track->album()->prettyName().isEmpty() )
            text += "\n (" + track->album()->prettyName() + ") ";
        else
            text += '\n';
        if( track->length() > 0 )
            text += Meta::msToPrettyTime( track->length() );
    }

    if( text.isEmpty() )
        text =  track->playableUrl().fileName();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    if( App::instance()->trayIcon() )
    {
        if( track && track->album() )
        {
            App::instance()->trayIcon()->setIconByPixmap( The::svgHandler()->imageWithBorder( track->album(), 100, 5 ) );
        }
        App::instance()->trayIcon()->showMessage( "Amarok", text, QString() );
    }

}
