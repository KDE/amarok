/****************************************************************************************
 * Copyright (c) 2010 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2010 Christian Wagner <christian.wagner86@gmx.at>                      *
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

#include "FileType.h"

using namespace Amarok;

QStringList FileTypeSupport::s_fileTypeStrings = QStringList()
        << QLatin1String( "" )
        << QLatin1String( "mp3" )
        << QLatin1String( "ogg" )
        << QLatin1String( "flac" )
        << QLatin1String( "mp4" )
        << QLatin1String( "wma" )
        << QLatin1String( "aiff" )
        << QLatin1String( "mpc" )
        << QLatin1String( "tta" )
        << QLatin1String( "wav" )
        << QLatin1String( "wv" )
        << QLatin1String( "m4a" )
        << QLatin1String( "m4v" )
        << QLatin1String( "mod" )
        << QLatin1String( "s3m" )
        << QLatin1String( "it" )
        << QLatin1String( "xm" )
        << QLatin1String( "spx" )
        << QLatin1String( "opus" );

QString
FileTypeSupport::toString( Amarok::FileType ft )
{
    return s_fileTypeStrings.at( ft );
}

QStringList
FileTypeSupport::possibleFileTypes()
{
    return s_fileTypeStrings;
}

Amarok::FileType
Amarok::FileTypeSupport::fileType( const QString &extension )
{
    QString ext = extension.toLower();
    for( int i = 1; i < s_fileTypeStrings.size(); i++ )
    {
        if( s_fileTypeStrings.at( i ).compare( ext ) == 0 )
            return Amarok::FileType( i );
    }
    return Amarok::Unknown;
}
