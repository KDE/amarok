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

#ifndef TRANSCODING_CONTROLLER_H
#define TRANSCODING_CONTROLLER_H

#include "core/amarokcore_export.h"
#include "core/support/Components.h"
#include "core/transcoding/TranscodingDefines.h"
#include "core/transcoding/TranscodingFormat.h"

#include <KProcess>

#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>


namespace Transcoding
{

/**
 * Singleton class that handles and wraps around the Transcoding architecture.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROKCORE_EXPORT Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller( QObject *parent = nullptr );
    ~Controller() override;

    /**
     * Return set of all encoders, available or not.
     */
    QSet<Encoder> allEncoders() const { QSet<Encoder> encoderSet(m_formats.uniqueKeys().begin(), m_formats.uniqueKeys().end()); return encoderSet; }

    /**
     * Return a set of all available encoders. You can use @see format() to get all
     * available formats.
     */
    QSet<Encoder> availableEncoders() const { return m_availableEncoders; }

    /**
     * Return pointer to format that encodes using @p encoder. You must ensure that
     * @param encoder is in @see allEncoders(). Always returns non-null pointer which
     * remains owned by Transcoding::Controller.
     */
    Format *format( Encoder encoder ) const;

private Q_SLOTS:
    void onAvailabilityVerified( int exitCode, QProcess::ExitStatus exitStatus );

private:
    QMultiMap<Encoder, Format *> m_formats; // due to Format being polymorphic, we must store pointers
    QSet<Encoder> m_availableEncoders;
};

} //namespace Transcoding

#endif //TRANSCODING_CONTROLLER_H
