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

#ifndef TRANSCODING_AACFORMAT_H
#define TRANSCODING_AACFORMAT_H

#include "core/transcoding/TranscodingFormat.h"

namespace Transcoding
{

/**
 * This class implements the interface for the libfaac AAC encoder through FFmpeg, as long
 * as FFmpeg is compiled with support for the proprietary FAAC library.
 * If a distro chooses to package FFmpeg without libfaac support, this codec simply won't
 * show up in Amarok's GUI.
 * Note: Unfortunately it seems that this is the only good enough option on GNU/Linux as
 * of August 2010. At any time, I'll be happy to be proven wrong about this and replace
 * FAAC with another encoder.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_CORE_EXPORT AacFormat : public Format
{
public:
    AacFormat();
    QString prettyName() const override;
    QString description() const override;
    QIcon icon() const override;
    QStringList ffmpegParameters( const Configuration &configuration ) const override;
    bool verifyAvailability( const QString &ffmpegOutput ) const override;
};

}

#endif // TRANSCODING_AACFORMAT_H
