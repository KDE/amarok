/****************************************************************************************
 * Copyright (c) 2013 Martin Brodbeck <martin@brodbeck-online.de>                       *
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

#ifndef TRANSCODING_OPUSFORMAT_H
#define TRANSCODING_OPUSFORMAT_H

#include "core/transcoding/TranscodingFormat.h"

namespace Transcoding
{

/**
 * This class implements the interface for the Opus encoder through FFmpeg. FFmpeg must
 * be compiled with support for the libopus library for this to work.
 * @author Martin Brodbeck <martin@brodbeck-online.de>
 */
class AMAROKCORE_EXPORT OpusFormat : public Format
{
public:
    OpusFormat();
    QString prettyName() const override;
    QString description() const override;
    QIcon icon() const override;
    QStringList ffmpegParameters( const Configuration &configuration ) const override;
    bool verifyAvailability( const QString &ffmpegOutput ) const override;

private:
    inline int toFfmpegBitrate( int setting ) const;
    QVector< int > m_validBitrates;
};

}

#endif // TRANSCODING_OPUSFORMAT_H
