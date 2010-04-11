/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_PLAYLISTSEARCHPROXY_H
#define AMAROK_PLAYLISTSEARCHPROXY_H

#include "ProxyBase.h"

namespace Playlist
{

/**
 * A ProxyModel that implements Playlist searching.
 * It should sit anywhere above the FilterProxy and SortProxy.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class SearchProxy : public ProxyBase
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit SearchProxy( AbstractModel *belowModel, QObject *parent = 0 );

    /**
     * Destructor.
     */
    ~SearchProxy();

    /**
     * Implementation of Playlist::AbstractModel: search/filter-related functions.
     */
    void clearSearchTerm();
    int currentSearchFields() { return m_currentSearchFields; }
    QString currentSearchTerm() { return m_currentSearchTerm; }
    int find( const QString & searchTerm, int searchFields = MatchTrack );
    int findNext( const QString & searchTerm, int selectedRow, int searchFields = MatchTrack   );
    int findPrevious( const QString & searchTerm, int selectedRow, int searchFields = MatchTrack  );

private:
    QString m_currentSearchTerm;
    int m_currentSearchFields;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSEARCHPROXY_H
