/***************************************************************************
 *   Copyright (C) 2003-2006 Mark Kretschmann <kretschmann@kde.org>        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CollectionScanner.h"
#include "metadata/tplugins.h"

#include <QFile>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>

int main( int argc, char *argv[] )
{
    const KAboutData about( "amarokcollectionscanner", "amarok",
    ki18n( "Amarok Collection Scanner\n\nNote: For debugging purposes this application can be invoked from the command line, but it will not actually build a collection this way." ), "0.1",
    ki18n( "Collection Scanner for Amarok" ), KAboutData::License_GPL,
    ki18n( "(C) 2003-2008, The Amarok Developers" ),
    ki18n( "IRC:\nserver: irc.freenode.net / channels: #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org" ),
    I18N_NOOP( "http://amarok.kde.org" ) );

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &about ); //calls KCmdLineArgs::addStdCmdLineOptions()

    KCmdLineOptions options;
    options.add("+Folder(s)", ki18n( "Folders to scan" ));
    options.add("r");
    options.add("recursive", ki18n( "Scan folders recursively" ));
    options.add("i");
    options.add("incremental", ki18n( "Incremental Scan (modified folders only)" ));
    options.add("p");
    options.add("importplaylists", ki18n( "Import playlist" ));
    options.add("s");
    options.add("restart", ki18n( "Restart the scanner at last position, after a crash" ));
    options.add("d");
    options.add("pid", ki18n( "PID of Amarok instance" ));
    options.add("collectionid <argument>", ki18n( "The SqlCollection instance to connect to. Must be set for incremental scans" ));
    KCmdLineArgs::addCmdLineOptions( options );  //add our own options

    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    // Parse list of folder arguments
    QStringList folders;
    for( int i = 0; i < args->count(); i++ )
        folders << args->arg( i );

    const bool recursive        = args->isSet( "recursive" );
    const bool incremental      = args->isSet( "incremental" );
    const bool importplaylists  = args->isSet( "importplaylists" );
    const bool restart          = args->isSet( "restart" );
    const QString pid           = args->getOption( "pid " );
    const QString collectionId  = args->getOption( "collectionid" );

    CollectionScanner scanner( folders, pid, collectionId, recursive, incremental, importplaylists, restart );

    registerTaglibPlugins();

    return scanner.exec();
}


