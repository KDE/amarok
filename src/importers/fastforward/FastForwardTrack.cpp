/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "FastForwardTrack.h"

#include "core/support/Debug.h"
#include "importers/ImporterSqlConnection.h"

using namespace StatSyncing;

FastForwardTrack::FastForwardTrack( const QString &trackUrl,
                                    const ImporterSqlConnectionPtr &connection,
                                    const Meta::FieldHash &metadata,
                                    const QSet<QString> &labels )
    : SimpleWritableTrack( metadata, labels )
    , m_connection( connection )
    , m_trackUrl( trackUrl )
{
}

FastForwardTrack::~FastForwardTrack()
{
}

void
StatSyncing::FastForwardTrack::doCommit( const qint64 fields )
{
    m_connection->transaction();
    bool ok = true;

    const QString query = QStringLiteral("SELECT deviceid, uniqueid FROM uniqueid WHERE url = :url");
    QVariantMap bindValues;
    bindValues.insert( QStringLiteral(":url"), m_trackUrl );

    const QList<QVariantList> result = m_connection->query( query, bindValues, &ok );
    if( !ok )
    {
        m_connection->rollback();
        return;
    }

    const int deviceId = result.front()[0].toInt();
    const QString uniqueId = result.front()[1].toString();

    QStringList updates;
    QVariantMap uBindValues;

    if( fields & Meta::valFirstPlayed )
    {
        updates << QStringLiteral("createdate = :createdate");
        uBindValues.insert( QStringLiteral(":createdate"), m_statistics.value( Meta::valFirstPlayed ) );
    }
    if( fields & Meta::valLastPlayed )
    {
        updates << QStringLiteral("accessdate = :accessdate");
        uBindValues.insert( QStringLiteral(":accessdate"), m_statistics.value( Meta::valLastPlayed ) );
    }
    if( fields & Meta::valRating )
    {
        updates << QStringLiteral("rating = :rating");
        uBindValues.insert( QStringLiteral(":rating"), m_statistics.value( Meta::valRating ) );
    }
    if( fields & Meta::valPlaycount )
    {
        updates << QStringLiteral("playcounter = :playcount");
        uBindValues.insert( QStringLiteral(":playcount"), m_statistics.value( Meta::valPlaycount ) );
    }

    if( !updates.isEmpty() )
    {
        const QString query = QStringLiteral("SELECT COUNT(*) FROM statistics WHERE url = :url");
        QVariantMap bindValues;
        bindValues.insert( QStringLiteral(":url"), m_trackUrl );

        const QList<QVariantList> result = m_connection->query( query, bindValues, &ok );
        if( !ok )
        {
            m_connection->rollback();
            return;
        }

        // Statistic row doesn't exist
        if( !result.front()[0].toInt() )
        {
            const QString query = QStringLiteral("INSERT INTO statistics (url, deviceid, uniqueid) "
                                  "VALUES ( :url, :devid, :uniqid )");
            QVariantMap bindValues;
            bindValues.insert( QStringLiteral(":url"), m_trackUrl );
            bindValues.insert( QStringLiteral(":devid"), deviceId );
            bindValues.insert( QStringLiteral(":url"), uniqueId );

            m_connection->query( query, bindValues, &ok );
            if( !ok )
            {
                m_connection->rollback();
                return;
            }
        }

        // Update statistics
        const QString uQuery = QStringLiteral("UPDATE statistics SET ") + updates.join(QStringLiteral(", ")) +
                               QStringLiteral(" WHERE url = :url");

        uBindValues.insert( QStringLiteral(":url"), m_trackUrl );
        m_connection->query( uQuery, uBindValues, &ok );
        if( !ok )
        {
            m_connection->rollback();
            return;
        }
    }

    if( fields & Meta::valLabel )
    {
        // Drop old label associations
        const QString query = QStringLiteral("DELETE FROM tags_labels WHERE url = :url");
        QVariantMap bindValues;
        bindValues.insert( QStringLiteral(":url"), m_trackUrl );
        m_connection->query( query, bindValues, &ok );
        if( !ok )
        {
            m_connection->rollback();
            return;
        }

        for( const QString &label : m_labels )
        {
            {
                // Check if the label exists
                const QString query = QStringLiteral("SELECT COUNT(*) FROM labels WHERE name = :name");
                QVariantMap bindValues;
                bindValues.insert( QStringLiteral(":name"), label );

                const QList<QVariantList> result = m_connection->query( query, bindValues,
                                                                        &ok );
                if( !ok )
                {
                    m_connection->rollback();
                    return;
                }

                // Insert label if it doesn't
                if( !result.front()[0].toInt() )
                {
                    const QString query = QStringLiteral("INSERT INTO labels (name, type) "
                                          "VALUES (:name, 1)");
                    QVariantMap bindValues;
                    bindValues.insert( QStringLiteral(":name"), label );

                    m_connection->query( query, bindValues, &ok );
                    if( !ok )
                    {
                        m_connection->rollback();
                        return;
                    }
                }
            }

            // Insert track <-> label association
            const QString query = QStringLiteral("INSERT INTO tags_labels (deviceid, url, uniqueid, "
                                  "labelid) VALUES ( :devid, :url, :uniqid, "
                                  "(SELECT id FROM labels WHERE name = :name) )");
            QVariantMap bindValues;
            bindValues.insert( QStringLiteral(":devid"), deviceId );
            bindValues.insert( QStringLiteral(":url"), m_trackUrl );
            bindValues.insert( QStringLiteral(":uniqid"), uniqueId );
            bindValues.insert( QStringLiteral(":name"), label );

            m_connection->query( query, bindValues, &ok );
            if( !ok )
            {
                m_connection->rollback();
                return;
            }
        }
    }

    m_connection->commit();
}
