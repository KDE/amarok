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

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>

int main( int argc, char *argv[] )
{
    QCoreApplication app( argc, argv );
    const KAboutData about( "amarokmp3tunesharmonydaemon",
                            "amarok",
                            "0.1",
                            i18n( "Amarok's MP3tunes Harmony Daemon" ),
                            KAboutLicense::GPL,
                            i18n( "(C) 2008, Casey Link" ),
                            i18n( "Handles AutoSync for the MP3tunes service in Amarok." ),
                            i18n( "IRC:\nserver: irc.libera.chat / channel: #amarok\n\nFeedback:\namarok@kde.org" ),
                            I18N_NOOP( "http://amarok.kde.org" ) );
    KAboutData::setApplicationData( about );

    QCommandLineParser parser;
    parser.setApplicationDescription( about.shortDescription() );
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption( { "identifier", i18n( "The identifier the daemon should use." ) } );
    parser.addOption( { "email", i18n( "The email to be used for authentication." ) } );
    parser.addOption( { "pin", i18n( "The pin to be used for authentication." ) } );
    parser.process( app );

    QString ident = parser.value( "identifier" );
    QString email = parser.value( "email" );
    QString pin = parser.value( "pin" );

    if( ident.isEmpty() )
        return -1;

    if( email.isEmpty() && pin.isEmpty() )
        theDaemon = new Mp3tunesHarmonyDaemon( ident, argc, argv );
    else
        theDaemon = new Mp3tunesHarmonyDaemon( ident, email, pin, argc, argv );

    Mp3tunesAmarokClient *client = new Mp3tunesAmarokClient();
    theDaemon->setClient( client );
    qDebug()  << "Starting main event loop";
    return QCoreApplication::exec();
}





