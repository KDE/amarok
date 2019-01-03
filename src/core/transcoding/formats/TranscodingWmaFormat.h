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

#ifndef TRANSCODING_WMAFORMAT_H
#define TRANSCODING_WMAFORMAT_H

#include "core/transcoding/TranscodingFormat.h"

namespace Transcoding
{

/**
 * This class implements the interface for the FFmpeg WMA codec. I wonder if anybody even
 * cares about WMA any more...
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_CORE_EXPORT WmaFormat : public Format
{
public:
    WmaFormat();
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

#endif // TRANSCODING_WMAFORMAT_H
