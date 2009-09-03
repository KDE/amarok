/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
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
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. This function emits found() or notFound() depending on
     * whether a match is found.
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    int find( const QString & searchTerm, int searchFields = MatchTrack );

    /**
     * Find the first track below a given row that matches the search term in one of the
     * specified search fields. This function emits found() or notFound() depending on
     * whether a match is found. If no row is found below the current row, the function wraps
     * around and returns the first match. If no match is found at all, -1 is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match below the offset, -1 if no match is found.
     */
    int findNext( const QString & searchTerm, int selectedRow, int searchFields = MatchTrack   );

    /**
     * Find the first track above a given row that matches the search term in one of the
     * specified search fields. This function emits found() or notFound() depending on
     * whether a match is found. If no row is found above the current row, the function wraps
     * around and returns the last match. If no match is found at all, -1 is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match above the offset, -1 if no match is found.
     */
    int findPrevious( const QString & searchTerm, int selectedRow, int searchFields = MatchTrack  );

    void clearSearchTerm();

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    QString currentSearchTerm() { return m_currentSearchTerm; }

    /**
     * Get the current search fields bit bitmask.
     * @return The current search fields.
     */
    int currentSearchFields() { return m_currentSearchFields; }

private:
    QString m_currentSearchTerm;
    int m_currentSearchFields;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSEARCHPROXY_H
