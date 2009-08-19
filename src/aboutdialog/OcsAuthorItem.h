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

#ifndef AMAROK_OCSAUTHORITEM_H
#define AMAROK_OCSAUTHORITEM_H

#include "libattica-ocsclient/ocsapi.h"

#include <KAboutPerson>

#include <QLabel>

class OcsAuthorItem : public QWidget
{
    Q_OBJECT
public:
    OcsAuthorItem( const KAboutPerson *person, const Attica::Person *ocsPerson, QWidget *parent = 0 );
    OcsAuthorItem( const KAboutPerson *person, QWidget *parent = 0 );
    virtual ~OcsAuthorItem();

private:
    void init();
    KAboutPerson *m_person;
    Attica::Person *m_ocsPerson;
    QLabel m_name;  //! Name 'Attribute' Surname (Nick)
    QLabel m_role;
    QLabel m_email;
    QLabel m_homepage;
    QLabel m_avatar;
    QLabel m_location; //! City, Country
    QLabel m_ircChannels;
    QLabel m_profile;
    /*
   <firstname>Frank</firstname>
   <lastname>Test</lastname>
   <communityrole>developer</communityrole>
   <homepage>opendesktop.org</homepage>
   <company>opendesktop.org</company>
   <avatarpic>http://www.KDE-Look.org/CONTENT/user-pics/0/Frank.jpg</avatarpic>
   <avatarpicfound>1</avatarpicfound>
   <bigavatarpic>http://www.KDE-Look.org/CONTENT/user-bigpics/0/Frank.jpg</bigavatarpic>
   <bigavatarpicfound>1</bigavatarpicfound>
   <city>Stuttgart</city>
   <country>Germany</country>
   <ircnick>karli</ircnick>
   <ircchannels>kde-dev, plasma</ircchannels>
   <irclink>irc://irc.freenode.org/kde-dev</irclink>
   <irclink>irc://irc.freenode.org/plasma</irclink>
   <profilepage>http://www.KDE-Look.org/usermanager/search.php?username=Frank</profilepage>
      */
};

#endif //AMAROK_OCSAUTHORITEM_H
