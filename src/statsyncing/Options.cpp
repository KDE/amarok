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

#include "Options.h"

using namespace StatSyncing;

Options::Options()
    : m_syncedFields( 0 )
{
}

qint64
Options::syncedFields() const
{
    return m_syncedFields;
}

void
Options::setSyncedFields( qint64 fields )
{
    m_syncedFields = fields;
}

QSet<QString>
Options::excludedLabels() const
{
    return m_excludedLabels;
}

void
Options::setExcludedLabels( const QSet<QString> &labels )
{
    m_excludedLabels = labels;
}
