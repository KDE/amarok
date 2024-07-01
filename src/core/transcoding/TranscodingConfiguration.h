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

#include "core/amarokcore_export.h"
#include "core/meta/Meta.h" // needed for default parameter constructor
#include "core/transcoding/TranscodingDefines.h"
#include "core/transcoding/TranscodingProperty.h"

#include <QMap>
#include <QVariant>

class KConfigGroup;

namespace Transcoding
{

/**
 * This class defines the values of a set of properties as described by an instance of
 * Transcoding::PropertyList. It contains all the data needed to start a Transcoding::Job.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROKCORE_EXPORT Configuration
{
public:

    //Normally we don't specify the numbers, but must specify enumeration values in
    //this case, as the values will get written to a config file, so we need to preserve
    //the numbers across versions and specify them explicitly
    enum TrackSelection {
        /**
         * Transcode all tracks
         */
        TranscodeAll = 0,
        /**
         * Transcode unless the target format is the same as source format
         */
        TranscodeUnlessSameType = 1,
        /**
         * Transcode tracks only if needed for playability in the destination collection
         */
        TranscodeOnlyIfNeeded = 2,
        /**
         * Transcode unless:
         * 1. the target format is the same as source format, AND
         * 2. the target bitrate is higher than source bitrate (with 10% tolerance to prevent re-encodings)
         */
        //TranscodeUnlessUpgradesBitrate = 3 //to be implemented.
    };

    explicit Configuration( Transcoding::Encoder encoder,
                            TrackSelection trackSelection = TranscodeAll );

    Encoder encoder() const { return m_encoder; }

    /**
     * Return true if this transcoding configuration is valid. JUST_COPY encoder is
     * treated as valid.
     */
    bool isValid() const { return m_encoder != INVALID; }

    /**
     * Return true if this configuration represents plain copying of files. Both INVALID
     * and JUST_COPY encoders are considered plain copying. Also returns true if an encoder is
     * selected, but the passed track does not satisfy the selected m_trackSelection value.
     */
    bool isJustCopy( const Meta::TrackPtr &srcTrack = Meta::TrackPtr(),
                     const QStringList &playableFileTypes = QStringList() ) const;

    QVariant property( const QByteArray &name ) const;
    void addProperty(const QByteArray &name, const QVariant &value );

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

    TrackSelection trackSelection() const { return m_trackSelection; }
    void setTrackSelection( TrackSelection trackSelection );

    bool operator!=( const Configuration &other ) const;

private:
    /**
     * Map an Encoder to its identifier
     */
    static const QMap<Encoder, QString> &encoderNames();

    /**
     * Return a user representable description of the format and the TrackSelection
     */
    QString formatPrettyPrefix() const;

    static QMap<Encoder, QString> s_encoderNames;
    Encoder m_encoder;
    QMap<QByteArray, QVariant> m_values;
    TrackSelection m_trackSelection; //the transcoding configuration
};

}

#endif //TRANSCODING_CONFIGURATION_H
