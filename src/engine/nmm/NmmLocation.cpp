/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "NmmLocation.h"

NmmLocation::NmmLocation()
{}

NmmLocation::NmmLocation( QString hostname, bool audio, bool video, int volume, int status )
  : m_hostname(hostname), 
    m_audio(audio), 
    m_video(video), 
    m_volume(volume),
    m_status( status)
{
}

NmmLocation::NmmLocation( const NmmLocation &old )
{
  m_hostname = old.hostname();
  m_audio = old.audio();
  m_video = old.video();
  m_volume = old.volume();
  m_status = old.status();
}

NmmLocation::~NmmLocation()
{}

QString NmmLocation::hostname() const
{
  return m_hostname;
}

void NmmLocation::setHostname(QString hostname)
{
  m_hostname = hostname;
}
