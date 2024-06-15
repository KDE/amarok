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

#include "TranscodingAlacFormat.h"

#include <KLocalizedString>

using namespace Transcoding;

AlacFormat::AlacFormat()
{
    m_encoder = ALAC;
    m_fileExtension = QStringLiteral("m4a");
    //ALAC seems to have absolutely no configurable options whatsoever. Gnomes would love it.
}

QString
AlacFormat::prettyName() const
{
    return i18n( "Apple Lossless" );
}

QString
AlacFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Apple_Lossless>Apple Lossless</a> "
                  "(ALAC) is an audio codec for lossless compression of digital music.<br>"
                  "Recommended only for Apple music players and players that do not support "
                  "FLAC." );
}

QIcon
AlacFormat::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-x-flac") ); //TODO: get a *real* icon!
}

QStringList
AlacFormat::ffmpegParameters( const Configuration &configuration ) const
{
    Q_UNUSED( configuration )
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("alac");
    parameters << QStringLiteral("-vn"); // no album art, writing it to m4a is not supported by ffmpeg
    return parameters;
}

bool
AlacFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... alac +") ) );
}
