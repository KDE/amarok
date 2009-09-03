/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef DEFINE_HARMONY
#define DEFINE_HARMONY
#endif
#include "Mp3tunesHarmonyDaemon.h"
#include "AmarokClient.h"
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <QApplication>
#include <QtDebug>

int main( int argc, char *argv[] )
{
    const KAboutData about( "amarokmp3tunesharmonydaemon", "amarok",
    ki18n( "Amarok's MP3tunes Harmony Daemon" ), "0.1",
    ki18n( "Handles AutoSync for the MP3tunes service in Amarok." ), KAboutData::License_GPL,
    ki18n( "(C) 2008, Casey Link" ),
    ki18n( "IRC:\nserver: irc.freenode.net / channels: #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org" ),
    I18N_NOOP( "http://amarok.kde.org" ) );

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &about ); //calls KCmdLineArgs::addStdCmdLineOptions()

    KCmdLineOptions options;
    options.add("+identifier", ki18n( "The identifier the daemon should use." ));
    options.add("+[email]", ki18n( "The email to be used for authentication." ));
    options.add("+[pin]", ki18n( "The pin to be used for authentication." ));
    KCmdLineArgs::addCmdLineOptions( options );  //add our own options

    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    QString ident;
    QString email;
    QString pin;
    if ( args->count() < 1 || args->count() > 3 )
        return -1;
    if( args->count() > 0 )
        ident = args->arg( 0 );
    if( args->count() == 3 )
    {
        email = args->arg( 1 );
        pin = args->arg( 2 );
    }

    if( email.isEmpty() && pin.isEmpty() )
        theDaemon = new Mp3tunesHarmonyDaemon( ident );
    else
        theDaemon = new Mp3tunesHarmonyDaemon( ident, email, pin );

    Mp3tunesAmarokClient *client = new Mp3tunesAmarokClient();
    theDaemon->setClient( client );
    qDebug()  << "Starting main event loop";
    QCoreApplication::exec();
}





