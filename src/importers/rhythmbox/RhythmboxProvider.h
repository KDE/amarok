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

#ifndef STATSYNCING_RHYTHMBOX_PROVIDER_H
#define STATSYNCING_RHYTHMBOX_PROVIDER_H

#include "importers/ImporterProvider.h"
#include "statsyncing/SimpleTrack.h"

class QXmlStreamReader;

namespace StatSyncing
{

class RhythmboxProvider : public ImporterProvider
{
public:
    RhythmboxProvider( const QVariantMap &config, ImporterManager *importer );
    ~RhythmboxProvider();

    qint64 reliableTrackMetaData() const;
    qint64 writableTrackStatsData() const;
    QSet<QString> artists();
    TrackList artistTracks( const QString &artistName );

private:
    void readXml( const QString &byArtist );
    void readRhythmdb( QXmlStreamReader &xml, const QString &byArtist );
    void readSong( QXmlStreamReader &xml, const QString &byArtist );
    QString readValue( QXmlStreamReader &xml );

    QSet<QString> m_artists;
    TrackList m_artistTracks;
};

class RhythmboxTrack : public SimpleTrack
{
public:
    RhythmboxTrack( const Meta::FieldHash &metadata );
    ~RhythmboxTrack();

    int rating() const;
};

} // namespace StatSyncing

#endif // STATSYNCING_RHYTHMBOX_PROVIDER_H
