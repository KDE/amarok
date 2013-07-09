/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef PARANOIAHELPER_H
#define PARANOIAHELPER_H

#include "meta/AudioCdTrack.h"

#include <QString>

#include <ThreadWeaver/Job>

#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>

/**
 * Helps copy a track out of CD using cdda-paranoia library.
 * Deletes itself after work is finished.
 */
class ParanoiaHelper:  public ThreadWeaver::Job
{
    Q_OBJECT
public:
    ParanoiaHelper( const QString &device, Meta::TrackPtr track, const QString& name );
    ~ParanoiaHelper()
    {
    };
    void run();

signals:
    /** Notifies that copying has been finished */
    void copyingDone( Meta::TrackPtr track, const QString &path, bool succesfull = true );
private:
    cdrom_drive* openDrive();
    QString m_device;
    /** track which has to be copyed*/
    Meta::TrackPtr m_track;
    /** filename for track */
    QString m_name;
};
#endif