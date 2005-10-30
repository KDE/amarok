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

#include <collectionscanner.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>


int main( int argc, char *argv[] )
{
    KAboutData about( "amarokcollectionscanner", "CollectionScanner", "0.1" );

        static KCmdLineOptions options[] =
        {
            { "+[Folder(s)]", I18N_NOOP( "Folders to scan" ), 0 },
            { "i", 0, 0 },
            { "incremental", I18N_NOOP( "Update scan" ), 0 },
            { "r", 0, 0 },
            { "recursive", I18N_NOOP( "Scan folders recursively" ), 0 },
            { "p", 0, 0 },
            { "importplaylists", I18N_NOOP( "Import playlist" ), 0 },
            { 0, 0, 0 }
        };


    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &about ); //calls KApplication::addCmdLineOptions()
    KCmdLineArgs::addCmdLineOptions( options );   //add our own options


    CollectionScanner cs( "/home/mark/mp3" );
    return cs.exec();
}


