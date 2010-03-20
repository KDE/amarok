/****************************************************************************************
 * Copyright (c) 2006 Sebastien Laout <slaout@linux62.org>                              *
 * Copyright (c) 2008,2009 Valerio Pilo <amroth@kmess.org>                              *
 * Copyright (c) 2008,2009 Sjors Gielen <sjors@kmess.org>                               *
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

#ifndef LIKEBACK_PRIVATE_H
#define LIKEBACK_PRIVATE_H

#include <KAction>
#include <KToggleAction>

#include <QTimer>


class QButtonGroup;


class LikeBackPrivate
{
public:
    LikeBackPrivate();
    ~LikeBackPrivate();

    LikeBackBar             *bar;
    KConfigGroup             config;
    const KAboutData        *aboutData;
    LikeBack::Button         buttons;
    QString                  hostName;
    QString                  remotePath;
    quint16                  hostPort;
    QStringList              acceptedLocales;
    LikeBack::WindowListing  windowListing;
    bool                     showBarByDefault;
    bool                     showBar;
    int                      disabledCount;
    QString                  fetchedEmail;
    KAction                 *sendAction;
    KToggleAction           *showBarAction;
};


// Constructor
LikeBackPrivate::LikeBackPrivate()
    : bar(0)
    , aboutData(0)
    , buttons(LikeBack::DefaultButtons)
    , hostName()
    , remotePath()
    , hostPort(80)
    , acceptedLocales()
    , windowListing(LikeBack::NoListing)
    , showBar(false)
    , disabledCount(0)
    , fetchedEmail()
    , sendAction(0)
    , showBarAction(0)
{
}


// Destructor
LikeBackPrivate::~LikeBackPrivate()
{
    delete bar;
    delete sendAction;
    delete showBarAction;

    aboutData = 0;
}

#endif // LIKEBACK_PRIVATE_H
