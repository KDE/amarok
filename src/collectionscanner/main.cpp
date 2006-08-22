/***************************************************************************
 *   Copyright (C) 2003-2006 by The Amarok Developers                      *
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

#include "collectionscanner.h"
#include "metadata/tplugins.h"

#include <qfile.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>

int main( int argc, char *argv[] )
{
    const KAboutData about( "amarokcollectionscanner",
    I18N_NOOP( "Amarok Collection Scanner\n\nNote: For debugging purposes this application can be invoked from the command line, but it will not actually build a collection this way." ), "0.1",
    I18N_NOOP( "Collection Scanner for Amarok" ), KAboutData::License_GPL,
    I18N_NOOP( "(C) 2003-2006, The Amarok Developers" ),
    I18N_NOOP( "IRC:\nserver: irc.freenode.net / channels: #amarok #amarok.de #amarok.es\n\nFeedback:\namarok@kde.org" ),
    I18N_NOOP( "http://amarok.kde.org" ) );


    static KCmdLineOptions options[] =
    {
        { "+Folder(s)", I18N_NOOP( "Folders to scan" ), 0 },
        { "r", 0, 0 },
        { "recursive", I18N_NOOP( "Scan folders recursively" ), 0 },
        { "i", 0, 0 },
        { "incremental", I18N_NOOP( "Incremental Scan (modified folders only)" ), 0 },
        { "p", 0, 0 },
        { "importplaylists", I18N_NOOP( "Import playlist" ), 0 },
        { "s", 0, 0 },
        { "restart", I18N_NOOP( "Restart the scanner at last position, after a crash" ), "" },
        { 0, 0, 0 }
    };


    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &about ); //calls KApplication::addCmdLineOptions()
    KCmdLineArgs::addCmdLineOptions( options );  //add our own options

    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    // Parse list of folder arguments
    QStringList folders;
    for( int i = 0; i < args->count(); i++ )
        folders << QFile::decodeName( args->arg( i ) );

    const bool recursive        = args->isSet( "recursive" );
    const bool incremental      = args->isSet( "incremental" );
    const bool importplaylists  = args->isSet( "importplaylists" );
    const bool restart          = args->isSet( "restart" );

    KApplication::disableAutoDcopRegistration();

    CollectionScanner scanner( folders, recursive, incremental, importplaylists, restart );

    registerTaglibPlugins();

    return scanner.exec();
}


