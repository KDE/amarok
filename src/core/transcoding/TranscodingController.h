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

#include "shared/amarok_export.h"
#include "TranscodingDefines.h"
#include "TranscodingFormat.h"
#include "core/support/Components.h"

#include <KProcess>

#include <QList>
#include <QObject>


namespace Transcoding
{

/**
 * Singleton class that handles and wraps around the Transcoding architecture.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_CORE_EXPORT Controller : public QObject
{
    Q_OBJECT
public:
    Controller( QObject *parent = 0 );

    const QList< Format * > & availableFormats() const { return m_availableFormats; }
    const QList< Format * > & allFormats() const { return m_formats; }
    const Format * format( Encoder encoder ) const;

private slots:
    void onAvailabilityVerified( int exitCode, QProcess::ExitStatus exitStatus );

private:
    const QList< Format * > m_formats;
    QList< Format * > m_availableFormats;   /* Unfortunately here I can't use QList< Format >
                                               instead of QList< Format * > because apparently
                                               QList can't maintain a structure of polymorphic
                                               stuff. This is an ugly workaround and it sucks.
                                                    -- Teo */
};

} //namespace Transcoding

#endif //TRANSCODING_CONTROLLER_H
