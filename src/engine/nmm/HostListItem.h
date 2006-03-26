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
#include <qpixmap.h>

class HostListItem : public KListViewItem {
  public:
    enum Column
    {
      Hostname = 0,
      Video,
      Audio,
      Volume,
      Status,
      Playback
    };

    HostListItem( QListView*, QString hostname, bool audio = true, bool video = true, int volume = 0, int status = 0, bool read_only = false);
    ~HostListItem();

    bool isVideoEnabled() const { return m_video; }
    void toggleVideo() { m_video = !m_video; }

    bool isAudioEnabled() const { return m_audio; }
    void toggleAudio() { m_audio = !m_audio; }

    void setStatus( int s ) { m_status = s; }
    int status() const { return m_status; }

    void setVolume( int v ) { m_volume = v; }
    int volume() const { return m_volume; }
    int volumeAtPosition( int );

    void updateColumn( int column ) const;

    /**
     * Shows extended status text in a QWhatsThis widget.
     * \todo handle different error scenarios
     */
    void statusToolTip();

    /**
     * Create detailed status message.
     * \todo make it user friendly/understandable
     * \todo right place for this method?
     */
    static QString prettyStatus( int );

  protected:
    void paintCell( QPainter * painter, const QColorGroup & cg, int column, int width, int align );

  private:
    enum { PixInset, PixLeft, PixRight };
    QPixmap* pixmapVolume( int );
    QPixmap generateGradient( int );
    
    bool m_audio;
    bool m_video;
    int m_volume; 
    int m_status;
    bool m_read_only;

};

#endif
