/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005-2006
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

#ifndef HOSTLISTITEM_H
#define HOSTLISTITEM_H

#include <klistview.h>

class HostListItem : public KListViewItem {
  public:
    enum Column
    {
      Hostname = 0,
      Video,
      Audio,
      Volume,
      Status
    };

    HostListItem( QListView*, QString hostname, bool audio = true, bool video = true, int volume = 0, bool read_only = false);
    ~HostListItem();

    bool isVideoEnabled() const { return m_video; }
    void toggleVideo() { m_video = !m_video; }

    bool isAudioEnabled() const { return m_audio; }
    void toggleAudio() { m_audio = !m_audio; }

    void updateColumn( int column ) const;

  protected:
    void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align );


  private:
    bool m_audio;
    bool m_video;
    bool m_read_only;
};

#endif
