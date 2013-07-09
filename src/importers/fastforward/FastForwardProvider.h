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

#ifndef STATSYNCING_FASTFORWARD_PROVIDER_H
#define STATSYNCING_FASTFORWARD_PROVIDER_H

#include "statsyncing/Provider.h"

#include "FastForwardConfig.h"

namespace StatSyncing
{

class FastForwardProvider : public Provider
{
    Q_OBJECT

public:
    FastForwardProvider( const FastForwardSettings &settings );
    virtual ~FastForwardProvider();

    virtual QString id() const;
    virtual QString prettyName() const;
    virtual QString description() const;
    virtual KIcon icon() const;
    virtual qint64 reliableTrackMetaData() const;
    virtual qint64 writableTrackStatsData() const;
    virtual Preference defaultPreference();

    /// Only called from non-main thread
    virtual QSet<QString> artists();

    /// Only called from non-main thread
    virtual TrackList artistTracks( const QString &artistName );

private:
    const FastForwardSettings m_settings;
    QSet<QString> m_artistsResult;
    TrackList m_artistTracksResult;

private slots:
    void artistsSearch();
    void artistTracksSearch( const QString &artistName );
};

} // namespace StatSyncing

#endif // STATSYNCING_FASTFORWARD_PROVIDER_H
