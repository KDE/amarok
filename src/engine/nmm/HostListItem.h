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

#include <qcolor.h>
#include <qlabel.h>
#include <qstring.h>
#include <qwidget.h>

class ServerregistryPing;
class PixmapToggleButton;

class HostListItem : public QWidget {
    Q_OBJECT
    
    public:
        HostListItem( QWidget *, QString _hostname, bool _audio = true, bool video = true, int volume = 0 );
        ~HostListItem();

        void setHighlighted( bool = true );
        QString hostname() const;

        bool isAudioEnabled() const;
        bool isVideoEnabled() const;

    protected:
        void mousePressEvent ( QMouseEvent * );

    signals:
        void pressed( HostListItem* );

    private slots:
        
        void registryAvailable( bool );

    private:
        /**
         * Calculates background color.
         */
        QColor calcBackgroundColor( QString, QColor );
        
        QLabel *statusButton;
        QLabel *hostLabel;

        PixmapToggleButton *audio;
        PixmapToggleButton *video;

        ServerregistryPing *registry;
};

#endif
