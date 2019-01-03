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

#ifndef STATSYNCING_ITUNES_PROVIDER_H
#define STATSYNCING_ITUNES_PROVIDER_H

#include "importers/ImporterProvider.h"

#include "MetaValues.h"

#include <QMutex>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace StatSyncing
{

class ITunesProvider : public ImporterProvider
{
    Q_OBJECT

public:
    ITunesProvider( const QVariantMap &config, ImporterManager *importer );
    ~ITunesProvider();

    qint64 reliableTrackMetaData() const override;
    qint64 writableTrackStatsData() const override;
    QSet<QString> artists() override;
    TrackList artistTracks( const QString &artistName ) override;

    void commitTracks() override;

private:
    void readXml( const QString &byArtist );
    void readPlist( QXmlStreamReader &xml, const QString &byArtist );
    void readTracks( QXmlStreamReader &xml, const QString &byArtist );
    void readTrack( QXmlStreamReader &xml, const QString &byArtist );
    QString readValue( QXmlStreamReader &xml );

    void writeTracks( QXmlStreamReader &reader, QXmlStreamWriter &writer,
                      const QMap<int, Meta::FieldHash> &dirtyData );
    void writeTrack( QXmlStreamReader &reader, QXmlStreamWriter &writer,
                     const Meta::FieldHash &dirtyData );

    QSet<QString> m_artists;
    TrackList m_artistTracks;
    QMap<int, Meta::FieldHash> m_dirtyData;
    QMutex m_dirtyMutex;

private Q_SLOTS:
    void trackUpdated( const int trackId, const Meta::FieldHash &statistics );
};

} // namespace StatSyncing

#endif // STATSYNCING_ITUNES_PROVIDER_H
