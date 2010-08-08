/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef TRANSCODING_CONFIGURATION_H
#define TRANSCODING_CONFIGURATION_H

#include "TranscodingDefines.h"
#include "TranscodingProperty.h"
#include "shared/amarok_export.h"

#include <QMap>
#include <QVariant>

namespace Transcoding
{

/**
 * This class defines the values of a set of properties as described by an instance of
 * Transcoding::PropertyList. It contains all the data needed to start a Transcoding::Job.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_EXPORT Configuration
{
public:
    explicit Configuration( Encoder encoder = NULL_CODEC );

    void addProperty( QByteArray name, QVariant value );

    Encoder encoder() const { return m_encoder; }

    QVariant property( QByteArray name ) const;

private:
    Encoder m_encoder;
    QMap< QByteArray, QVariant > m_values;
};

}

#endif //TRANSCODING_CONFIGURATION_H
