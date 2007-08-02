/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "XesamCollectionBuilder.h"

#include "debug.h"
#include "mountpointmanager.h"
#include "sqlcollection.h"
#include "XesamDbus.h"

#include <QBuffer>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDir>
#include <QXmlStreamWriter>

#include <kurl.h>

static const QString XESAM_NS = "";

#define DEBUG_XML true

XesamCollectionBuilder::XesamCollectionBuilder( SqlCollection *collection )
    : QObject( collection )
    , m_collection( collection )
{
    DEBUG_BLOCK
    m_xesam = new OrgFreedesktopXesamSearchInterface( "org.freedesktop.xesam.searcher",
                                                  "/org/freedesktop/xesam/searcher/main",
                                                  QDBusConnection::sessionBus() );
    if( m_xesam->isValid() )
    {
        connect( m_xesam, SIGNAL( HitsAdded( QString , int ) ), SLOT( slotHitsAdded( QString, int ) ) );
        connect( m_xesam, SIGNAL( HitsModified( QString, QList<int> ) ), SLOT( slotHitsModified( QString, QList<int> ) ) );
        connect( m_xesam, SIGNAL( HitsRemoved( QString, QList<int> ) ), SLOT( slotHitsRemoved( QString, QList<int> ) ) );
        QDBusReply<QString> sessionId = m_xesam->NewSession();
        if( !sessionId.isValid() )
        {
            debug() << "Could not acquire Xesam session, aborting";
            return;
            //TODO error handling
        }
        m_session = sessionId.value();
        if( !setupXesam() )
        {
            debug() << "Warning, could not setup xesam correctly";
        }
        QDBusReply<QString> search = m_xesam->NewSearch( m_session, generateXesamQuery() );
        if( search.isValid() )
        {
            m_search = search.value();
            m_xesam->StartSearch( m_search );
        }
        else
        {
            debug() << "Invalid response for NewSearch";
        }
    }
    else
    {
        //TODO display warning about unavailable xesam daemon
    }

}

XesamCollectionBuilder::~XesamCollectionBuilder()
{
    if( m_xesam && m_xesam->isValid() )
        m_xesam->CloseSession( m_session );
}

bool
XesamCollectionBuilder::setupXesam()
{
    bool status = true;
    if( !m_xesam->SetProperty( m_session, "search.live", QDBusVariant( true ) ).value().variant().toBool() )
    {
        debug() << "could not select xesam live search mode";
        status = false;
    }
    QStringList fields;
    fields << "uri" << "audio.title" << "audio.album" << "audio.artist" << "content.genre";
    fields << "audio.composer" << "audio.year" << "audio.comment" << "media.codec";
    m_xesam->SetProperty( m_session, "hit.fields", QDBusVariant( fields ) );
    QStringList fieldsExtended;
    m_xesam->SetProperty( m_session, "hit.fields.extended", QDBusVariant( fieldsExtended ) );
    m_xesam->SetProperty( m_session, "sort.primary", QDBusVariant( "uri" ) );
    m_xesam->SetProperty( m_session, "search.blocking", QDBusVariant( false ) );
    return status;
}

void
XesamCollectionBuilder::slotHitsAdded( const QString &search, int count )
{
    DEBUG_BLOCK
    debug() << "New Xesam hits: " << count;
    QDBusReply<VariantListVector> reply = m_xesam->GetHits( m_search, count );
    if( reply.isValid() )
    {
        VariantListVector result = reply.value();
        if( result.isEmpty() )
            return;
        KUrl firstUrl( result[0][0].toString() );
        QString dir = firstUrl.directory();
        QList<QList<QVariant> > dirData;
        //rows are sorted by directory/uri
        foreach( QList<QVariant> row, result )
        {
            KUrl url( row[0].toString() );
            if( url.directory() == dir )
            {
                dirData.append( row );
            }
            else
            {
                processDirectory( dirData );
                dirData.clear();
                dir = url.directory();
            }
        }
    }
}

void
XesamCollectionBuilder::slotHitsModified( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
}

void
XesamCollectionBuilder::slotHitsRemoved( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
}

QString
XesamCollectionBuilder::generateXesamQuery() const
{
    QStringList collectionFolders = MountPointManager::instance()->collectionFolders();
    QString result;
    QXmlStreamWriter writer( &result );
    writer.setAutoFormatting( DEBUG_XML );
    writer.writeStartElement( XESAM_NS, "request" );
    writer.writeStartElement( XESAM_NS, "query" );
    writer.writeAttribute( XESAM_NS, "type", "music" );
    if( collectionFolders.size() <= 1 )
    {
        QString folder = collectionFolders.isEmpty() ? QDir::homePath() : collectionFolders[0];
        writer.writeStartElement( XESAM_NS, "startsWith" );
        writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
        writer.writeTextElement( XESAM_NS, "string", folder );
        writer.writeEndElement();
    }
    else
    {
        writer.writeStartElement( XESAM_NS, "or" );
        foreach( QString folder, collectionFolders )
        {
            writer.writeStartElement( XESAM_NS, "startsWith" );
            writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
            writer.writeTextElement( XESAM_NS, "string", folder );
            writer.writeEndElement();
        }
        writer.writeEndElement();
    }
    writer.writeEndDocument();
    if( DEBUG_XML )
        debug() << result;
    return result;
}

void
XesamCollectionBuilder::processDirectory( const QList<QList<QVariant> > &data )
{
    QSet<QString> artists;
    foreach(QList<QVariant> row, data )
    {
        
    }
}

int
XesamCollectionBuilder::artistId( const QString &artist )
{
    if( m_artists.contains( artist ) )
        return m_artists.value( artist );
    QString query = QString( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_collection->escape( artist ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums( name ) VALUES ('%1');" ).arg( m_collection->escape( artist ) );
        int id = m_collection->insert( insert, "albums" );
        m_artists.insert( artist, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_artists.insert( artist, id );
        return id;
    }
}

int
XesamCollectionBuilder::genreId( const QString &genre )
{
    if( m_genre.contains( genre ) )
        return m_genre.value( genre );
    QString query = QString( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_collection->escape( genre ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_collection->escape( genre ) );
        int id = m_collection->insert( insert, "genre" );
        m_genre.insert( genre, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_genre.insert( genre, id );
        return id;
    }
}

int
XesamCollectionBuilder::composerId( const QString &composer )
{
    if( m_composer.contains( composer ) )
        return m_composer.value( composer );
    QString query = QString( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_collection->escape( composer ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_collection->escape( composer ) );
        int id = m_collection->insert( insert, "composers" );
        m_composer.insert( composer, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_composer.insert( composer, id );
        return id;
    }
}

#include "XesamCollectionBuilder.moc"
