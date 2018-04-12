/****************************************************************************************
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_PRETTY_TREE_ROLES_H
#define AMAROK_PRETTY_TREE_ROLES_H

namespace PrettyTreeRoles
{
    /** Roles used for the PrettyTreeDelegate and some models.
        The numbers start at the strange index to reduce the possibility that
        different roles from different models clash.
    */
    enum CustomRolesId
    {
        SortRole = Qt::UserRole + 51,
        FilterRole = Qt::UserRole + 52,
        ByLineRole = Qt::UserRole + 53,
        /** Boolean value whether given collection knows about used and total capacity */
        HasCapacityRole = Qt::UserRole + 54,
        /** Number of bytes used by music and other files in collection (float) */
        UsedCapacityRole = Qt::UserRole + 55,
        /** Total capacity of the collection in bytes (float) */
        TotalCapacityRole = Qt::UserRole + 56,
        /** The number of collection actions */
        DecoratorRoleCount = Qt::UserRole + 57,

        /** The collection actions */
        DecoratorRole = Qt::UserRole + 58,

        /** True if the item has a cover that should be displayed */
        HasCoverRole = Qt::UserRole + 59,

        YearRole = Qt::UserRole + 60
    };
}

#endif
