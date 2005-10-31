/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
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

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>


int main( int argc, char *argv[] )
{
    const KAboutData about( "amarokcollectionscanner",
    I18N_NOOP( "amaroK Collection Scanner" ), "0.1",
    I18N_NOOP( "Collection Scanner for amaroK" ), KAboutData::License_GPL,
    I18N_NOOP( "(C) 2003-2005, The amaroK Developers" ),
    I18N_NOOP( "IRC:\nserver: irc.freenode.net / channels: #amarok #amarok.de\n\nFeedback:\namarok-devel@lists.sourceforge.net" ),
    I18N_NOOP( "http://amarok.kde.org" ) );


    static KCmdLineOptions options[] =
    {
        { "+[Folder(s)]", I18N_NOOP( "Folders to scan" ), 0 },
        { "r", 0, 0 },
        { "recursive", I18N_NOOP( "Scan folders recursively" ), 0 },
        { "i", 0, 0 },
        { "importplaylists", I18N_NOOP( "Import playlist" ), 0 },
        { 0, 0, 0 }
    };


    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &about ); //calls KApplication::addCmdLineOptions()
    KCmdLineArgs::addCmdLineOptions( options );  //add our own options

    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    // Parse list of folder arguments
    QStringList folders;
    for( int i = 0; i < args->count(); i++ )
        folders << args->arg( i );

    const bool recursive        = args->isSet( "recursive" );
    const bool importplaylists  = args->isSet( "importplaylists" );


    CollectionScanner scanner( folders, recursive, importplaylists );
    return scanner.exec();
}


