/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef IPHONEMOUNTPOINT_H
#define IPHONEMOUNTPOINT_H

#include <QString>


/**
 * An automatic iPhone/iPad mountpoint that tries to mount the device using ifuse in
 * constructor and to unmount it in destructor.
 */
class IphoneMountPoint
{
    public:
        /**
         * Mount iPhone/iPad device by its 40-digit device UUID or mount any connected
         * iPhone/iPad if @param uuid is empty.
         */
        IphoneMountPoint( const QString &uuid );
        ~IphoneMountPoint();

        /**
         * Get location where iPhone was mounted to. If empty, mounting the iPhone failed.
         */
        QString mountPoint() const;

    private:
        Q_DISABLE_COPY(IphoneMountPoint)

        /**
         * Creates unique directory for mounting iPhone under temporary directory
         */
        QString constructMountpoint( const QString &uuid );

        QString m_mountPoint;
};

#endif // IPHONEMOUNTPOINT_H
