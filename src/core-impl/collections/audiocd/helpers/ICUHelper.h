/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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
#ifndef ICU_HELPER_H
#define ICU_HELPER_H

#include <QByteArray>
#include <QVector>
#include <QSet>

class ICUHelper
{
public:
    bool detectAllEncodings( QVector<QString> &encodings ) const;
    void addSample( const char* sample )
    {
        m_samples.append( sample );
    }

private:
    QByteArray m_samples;
};
#endif
