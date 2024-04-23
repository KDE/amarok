/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "covermanager/CoverCache.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"

#include <QDir>
#include <QImage>
#include <QMutexLocker>
#include <QPixmapCache>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QStandardPaths>
#include <QWriteLocker>

#include <KLocalizedString>

#include <thread>

CoverCache* CoverCache::s_instance = nullptr;

CoverCache*
CoverCache::instance()
{
    return s_instance ? s_instance : (s_instance =  new CoverCache());
}

void CoverCache::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

CoverCache::CoverCache()
{ }

CoverCache::~CoverCache()
{
    m_lock.lockForWrite();
    m_lock.unlock();
}

void
CoverCache::invalidateAlbum( const Meta::Album* album )
{
    if( !s_instance )
        return;

    QWriteLocker locker( &s_instance->m_lock );

    if( !s_instance->m_keys.contains( album ) )
        return;

    CoverKeys allKeys = s_instance->m_keys.take( album );
    foreach( const QPixmapCache::Key &key, allKeys.values() )
    {
        QPixmapCache::remove( key );
    }
}

QPixmap
CoverCache::getCover( const Meta::AlbumPtr &album, int size ) const
{
    QPixmap pixmap;

    if( size > 1 ) // full size covers are not cached
    {
        QReadLocker locker( &m_lock );
        const CoverKeys &allKeys = m_keys.value( album.data() );
        if( !allKeys.isEmpty() )
        {
            QPixmapCache::Key key = allKeys.value( size );
            if( key != QPixmapCache::Key() && QPixmapCache::find( key, &pixmap ) )
                return pixmap;
        }
    }

    QImage image = album->image( size );

    // -- get the null cover if someone really wants to have a pixmap
    // might be no cover, since there doesn't seem to be hasCover for album any more
    if( image.isNull() )
    {
        const QDir &cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
        if( size <= 1 )
            size = 500;
        const QString &noCoverKey = QString::number( size ) + "@nocover.png";

        QPixmap pixmap;
        // look in the memory pixmap cache
        if( QPixmapCache::find( noCoverKey, &pixmap ) )
            return pixmap;

        if( cacheCoverDir.exists( noCoverKey ) )
        {
            pixmap.load( cacheCoverDir.filePath( noCoverKey ) );
        }
        else
        {
            const QPixmap orgPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/nocover.png" ) );
            pixmap = orgPixmap.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
            std::thread thread( QOverload<const QString&, const char*, int>::of( &QPixmap::save ), pixmap, cacheCoverDir.filePath( noCoverKey ), "PNG", -1 );
            thread.detach();
        }
        QPixmapCache::insert( noCoverKey, pixmap );
        return pixmap;
    }

    pixmap = QPixmap::fromImage( image );

    // -- add the cover to the cache if not full-scale or too big
    if( size > 1 && size < 1000 )
    {
        // sadly I can't relock for write
        QWriteLocker locker( &m_lock );

        QPixmapCache::Key key = QPixmapCache::insert( pixmap );
        m_keys[ album.data() ][ size ] = key;
    }

    return pixmap;
}

CoverCache* The::coverCache()
{
    return CoverCache::instance();
}

