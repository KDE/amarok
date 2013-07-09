/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef METADATAHELPER_H
#define METADATAHELPER_H

#include <KSharedPtr>
#include <QString>
#include <cdio/cdio.h>

#include "ICUHelper.h"

class EntityInfo;

/**
 * Base class for all metadata sources
 */
class MetaDataHelper: public QSharedData
{
    public:
        MetaDataHelper( const QString& encodingPreferences );
        virtual ~MetaDataHelper();
        virtual bool isAvailable() const = 0;
        virtual EntityInfo getDiscInfo() const = 0;
        virtual EntityInfo getTrackInfo( track_t trackNum ) const = 0;
        virtual QByteArray getRawDiscTitle() const = 0;
        void getEncodings( QVector<QString> &encodings ) const;

    protected:
        /** Tries to detect the data encoding and set it*/
        QString encode( const char* field ) const;
        mutable ICUHelper m_icu;
        QString m_encodingPreferences;
};

typedef KSharedPtr<MetaDataHelper> MetaDataHelperPtr;
#endif
