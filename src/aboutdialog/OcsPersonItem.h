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

#ifndef AMAROK_OCSPERSONITEM_H
#define AMAROK_OCSPERSONITEM_H

#include "ui_OcsPersonItem.h"

#include "libattica-ocsclient/ocsapi.h"

#include <KAboutPerson>
#include <KToolBar>

#include <QLabel>

class OcsPersonItem : public QWidget, private Ui::OcsPersonItem
{
    Q_OBJECT

public:
    enum PersonStatus
    {
        Author = 0,
        Contributor = 1
    };

    OcsPersonItem( const KAboutPerson &person, const Attica::Person &ocsPerson, PersonStatus status, QWidget *parent = 0 );
    OcsPersonItem( const KAboutPerson &person, PersonStatus status, QWidget *parent = 0 );

    virtual ~OcsPersonItem();

    QString name();

private slots:
    void launchUrl( QAction *action );

private:
    void init();
    const KAboutPerson *m_person;
    const Attica::Person *m_ocsPerson;
    QString m_aboutText;
    KToolBar *m_iconsBar;
    PersonStatus m_status;
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

#endif //AMAROK_OCSPERSONITEM_H
