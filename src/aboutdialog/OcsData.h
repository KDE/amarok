/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_OCSDATA_H
#define AMAROK_OCSDATA_H

#include "Amarok.h"
#include "amarok_export.h"

#include "kaboutdata.h"

#include <QList>
#include <QPair>
#include <QString>

class AMAROK_EXPORT OcsData
{
public:
    typedef QList< QPair< QString, KAboutPerson > > OcsPersonList;

    OcsData();
    virtual ~OcsData();
    void addAuthor( const QString &username, const KAboutPerson &person );
    void addCredit( const QString &username, const KAboutPerson &person );

    const OcsPersonList * authors() const { return &m_authors; }
    const OcsPersonList * credits() const { return &m_credits; }

private:
    QList< QPair< QString, KAboutPerson > > m_authors;
    QList< QPair< QString, KAboutPerson > > m_credits;
};

#endif //AMAROK_OCSDATA_H
