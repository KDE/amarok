/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#ifndef AMAROK_AMAROKMIMEDATA_H
#define AMAROK_AMAROKMIMEDATA_H

#include "amarok_export.h"
#include "meta/meta.h"
#include "collection/querymaker.h"

#include <QMimeData>

class AMAROK_EXPORT AmarokMimeData : public QMimeData
{
    Q_OBJECT
    public:
        AmarokMimeData();
        virtual ~AmarokMimeData();

        virtual QStringList formats() const;
        virtual bool hasFormat( const QString &mimeType ) const;

        Meta::TrackList tracks() const;
        void setTracks( const Meta::TrackList &tracks );

        QueryMaker* queryMaker();
        void setQueryMaker( QueryMaker *queryMaker );

    protected:
        virtual QVariant retrieveData( const QString &mimeType, QVariant::Type type );

    private:
        Meta::TrackList m_tracks;
        QueryMaker *m_queryMaker;
        bool m_deleteQueryMaker;

};


#endif
