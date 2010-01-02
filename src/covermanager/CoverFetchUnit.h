/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_COVERFETCHUNIT_H
#define AMAROK_COVERFETCHUNIT_H

#include "meta/Meta.h"

#include <KSharedPtr>

class CoverFetchPayload;

/**
 * A work unit for the cover fetcher queue.
 */
class CoverFetchUnit : public QSharedData
{
public:
    typedef KSharedPtr< CoverFetchUnit > Ptr;

    CoverFetchUnit( Meta::AlbumPtr album, const CoverFetchPayload *url, bool interactive = false );
    CoverFetchUnit( const CoverFetchUnit &cpy );
    explicit CoverFetchUnit() {}
    ~CoverFetchUnit();

    Meta::AlbumPtr album() const;
    const QStringList &errors() const;
    const CoverFetchPayload *url() const;

    bool isInteractive() const;

    template< typename T >
        void addError( const T &error );

    CoverFetchUnit &operator=( const CoverFetchUnit &rhs );
    bool operator==( const CoverFetchUnit &other ) const;
    bool operator!=( const CoverFetchUnit &other ) const;

private:
    Meta::AlbumPtr m_album;
    QStringList m_errors;
    bool m_interactive;
    const CoverFetchPayload *m_url;
};

#endif /* AMAROK_COVERFETCHUNIT_H */
