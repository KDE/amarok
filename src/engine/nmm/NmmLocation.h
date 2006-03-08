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

#ifndef NMMLOCATION_H
#define NMMLOCATION_H

#include <qstring.h>

class NmmLocation {
  public:
    NmmLocation();
    NmmLocation(QString hostname, bool audio, bool video, int volume, int status );
    NmmLocation(const NmmLocation&);
    ~NmmLocation();

    QString hostname() const;
    void setHostname(QString);

    bool audio() const { return m_audio; }
    void setAudio( bool audio ) { m_audio = audio; }

    bool video() const { return m_video; } 
    void setVideo( bool video ) { m_video = video; }

    int volume() const { return m_volume; }
    void setVolume( int v ) { m_volume = v; }

    void setStatus( int s ) { m_status = s; }
    int status() const { return m_status; }

  private:
    QString m_hostname;
    bool m_audio;
    bool m_video;
    int m_volume;
    int m_status;
};

#endif
