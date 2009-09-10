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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_OCSPERSONITEM_H
#define AMAROK_OCSPERSONITEM_H

#include "ui_OcsPersonItem.h"

#include "libattica-ocsclient/person.h"
#include "libattica-ocsclient/provider.h"
#include "OcsData.h"

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
    enum State
    {
        Offline = 0,
        Online = 1
    };

    OcsPersonItem( const KAboutPerson &person, const QString ocsUsername, PersonStatus status, QWidget *parent = 0 );

    virtual ~OcsPersonItem();

    QString name();

    void switchToOcs( const Attica::Provider &provider );

signals:
    void ocsFetchStarted();
    void ocsFetchResult( int err );

private slots:
    void launchUrl( QAction *action );
    void onJobFinished( KJob *job );

private:
    void init();
    void fillOcsData( const Attica::Person &ocsPerson );
    const KAboutPerson *m_person;
    QString m_ocsUsername;
    QString m_aboutText;
    KToolBar *m_iconsBar;   //! holds the icons for email, homepage and oD.o profile
    KToolBar *m_snBar;      //! holds any other icons for social network profiles
    PersonStatus m_status;
    State m_state;
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
