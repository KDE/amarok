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

#include "TranscodingNullFormat.h"

#include <KLocale>

using namespace Transcoding;

NullFormat::NullFormat( const Encoder &encoder )
{
    m_encoder = encoder;
    m_fileExtension = "";
}

QString
NullFormat::prettyName() const
{
    return QString();
}

QString
NullFormat::description() const
{
    return QString();
}

QIcon
NullFormat::icon() const
{
    return QIcon();
}

QStringList
NullFormat::ffmpegParameters( const Configuration &configuration ) const
{
    Q_UNUSED( configuration )
    return QStringList() << "-acodec" << "copy";
}

bool
NullFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    Q_UNUSED( ffmpegOutput )
    return false;
}
