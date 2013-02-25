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
#include "core/amarokcore_export.h"

#include <KConfigGroup>

#include <QMap>
#include <QVariant>

namespace Transcoding
{

/**
 * This class defines the values of a set of properties as described by an instance of
 * Transcoding::PropertyList. It contains all the data needed to start a Transcoding::Job.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_CORE_EXPORT Configuration
{
public:
    explicit Configuration( Encoder encoder );

    Encoder encoder() const { return m_encoder; }

    /**
     * Return true if this transcoding configuration is valid. JUST_COPY encoder is
     * treated as valid.
     */
    bool isValid() const { return m_encoder != INVALID; }

    /**
     * Return true if this configuration represents plain copying of files. Bost INVALID
     * and JUST_COPY encoders are considered plain copying.
     */
    bool isJustCopy() const { return m_encoder == INVALID || m_encoder == JUST_COPY; }

    QVariant property( QByteArray name ) const;
    void addProperty( QByteArray name, QVariant value );

    /**
     * Re-create transcoding configuration from serialized form stored in a KConfigGroup.
     * Return invalid configuration if parsing failed or was incomplete.
     */
    static Configuration fromConfigGroup( const KConfigGroup &serialized );

    /**
     * Serialize this transcoding configuration to a KConfigGroup. All existing keys in
     * the group are erased or replaced.
     */
    void saveToConfigGroup( KConfigGroup &group ) const;

    /**
     * Return user-presentable representation of this configuration's codec and its
     * parameters.
     */
    QString prettyName() const;

private:
    /**
     * Get an Encoder to its identifier map
     */
    static const QMap<Encoder, QString> &encoderNames();

    static QMap<Encoder, QString> s_encoderNames;
    Encoder m_encoder;
    QMap<QByteArray, QVariant> m_values;
};

}

#endif //TRANSCODING_CONFIGURATION_H
