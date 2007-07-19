/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "pmpbackend.h"

#include <QCoreApplication>


PMPBackend::PMPBackend()
            : QObject()
            , m_udi()
{
}

PMPBackend::~PMPBackend()
{
}

void
PMPBackend::setUdi( const QString &udi )
{
    Q_UNUSED( udi );
}

QString
PMPBackend::getFilePath( const KUrl &url )
{
    QString path = url.path( KUrl::RemoveTrailingSlash );

    while( path[0] == '/' )
        path.remove( 0, 1 );

    int index = path.indexOf( '/' );
    QString filePath;
    if( index != -1 )
        filePath = path.remove( 0, index + 1 ) ;
    return filePath;
}


#include "pmpbackend.moc"

