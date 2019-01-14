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

#ifndef TRANSCODING_MP3FORMAT_H
#define TRANSCODING_MP3FORMAT_H

#include "core/transcoding/TranscodingFormat.h"

namespace Transcoding
{

/**
 * This class implements the interface for the LAME Mp3 encoder through FFmpeg. FFmpeg must
 * be compiled with support for the libmp3lame library for this to work. While hopelessly
 * patent encumbered (like any Free MP3 encoder), LAME is Free Software under the LGPL so
 * requiring LAME support in FFmpeg should not be an excessive demand.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROKCORE_EXPORT Mp3Format : public Format
{
public:
    Mp3Format();
    QString prettyName() const override;
    QString description() const override;
    QIcon icon() const override;
    QStringList ffmpegParameters( const Configuration &configuration ) const override;
    bool verifyAvailability( const QString &ffmpegOutput ) const override;
};

}

#endif // TRANSCODING_MP3FORMAT_H
