/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "OcsData.h"

#include <kaboutdata.h>
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

    OcsPersonItem( const KAboutPerson &person, const QString &ocsUsername, PersonStatus status, QWidget *parent = nullptr );

    ~OcsPersonItem() override;

    QString name() const;

Q_SIGNALS:
    void ocsFetchStarted();
    void ocsFetchResult( int err );

private Q_SLOTS:
    void launchUrl( QAction *action );

private:
    void init();
    const KAboutPerson *m_person;
    QString m_ocsUsername;
    QString m_aboutText;
    KToolBar *m_iconsBar;   //!< holds the icons for email, homepage and oD.o profile
    KToolBar *m_snBar;      //!< holds any other icons for social network profiles
    PersonStatus m_status;
};

#endif //AMAROK_OCSPERSONITEM_H
