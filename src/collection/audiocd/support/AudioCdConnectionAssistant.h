/****************************************************************************************
 * Copyright (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AUDIOCDCONNECTIONASSISTANT_H
#define AUDIOCDCONNECTIONASSISTANT_H

#include "ConnectionAssistant.h"

#include <QObject>

class AudioCdConnectionAssistant : public ConnectionAssistant
{
    Q_OBJECT
    
public:
    virtual ~AudioCdConnectionAssistant();

    virtual bool identify( const QString& udi );
    virtual MediaDeviceInfo* deviceInfo( const QString& udi );

};

#endif // AUDIOCDCONNECTIONASSISTANT_H
