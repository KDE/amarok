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

#ifndef HOSTLIST_H
#define HOSTLIST_H

#include <klistview.h>

class HostListItem;

class HostList : public KListView
{
  Q_OBJECT

  public:
    HostList( QWidget*, const char* );
    ~HostList();

    void setReadOnly( bool read_only ) { m_read_only = read_only; }
    bool readOnly() const { return m_read_only; }

    void notifyHostError( QString, int);

    friend class HostListItem;

  signals:
    /**
     * Emitted when audio of video toggle changes.
     */
    void viewChanged();

  protected slots:
    void contentsMousePressEvent( QMouseEvent *e );
    void contentsMouseMoveEvent( QMouseEvent *e = 0 );
    void leaveEvent( QEvent *e );

  private:
    bool m_read_only;
    HostListItem  *m_hoveredVolume; //if the mouse is hovering over the volume of an item
};

#endif 
