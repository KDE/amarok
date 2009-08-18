/****************************************************************************************
 * Copyright (c) 2007 Urs Wolfer <uwolfer at kde.org>                                   *
 * Copyright (c) 2008 Friedrich W. H. Kossebau <kossebau@kde.org>                       *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * Parts of this class have been take from the KAboutApplication class, which was       *
 * Copyright (c) 2000 Waldo Bastian (bastian@kde.org) and Espen Sand (espen@kde.org)    *
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

#ifndef AMAROK_ABOUTDIALOG_H
#define AMAROK_ABOUTDIALOG_H

#include "Amarok.h"

#include <kdialog.h>

class AMAROK_EXPORT ExtendedAboutDialog : public KDialog
{
    Q_OBJECT
public:
    explicit ExtendedAboutDialog( const KAboutData *aboutData, QWidget *parent = 0 );
    virtual ~ExtendedAboutDialog();

private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_showLicense(const QString&) )

        Q_DISABLE_COPY( ExtendedAboutDialog )
};

#endif  //AMAROK_ABOUTDIALOG_H
