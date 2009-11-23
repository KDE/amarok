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

#ifndef AMAROK_OCSPERSONLISTWIDGET_H
#define AMAROK_OCSPERSONLISTWIDGET_H

#include "OcsPersonItem.h"
#include "OcsData.h"
#include "libattica-ocsclient/provider.h"

class OcsPersonListWidget : public QWidget
{
    Q_OBJECT

public:
    OcsPersonListWidget( const QList< KAboutPerson > &persons,
                         const OcsData::OcsPersonList *ocsPersons,
                         OcsPersonItem::PersonStatus status = OcsPersonItem::Author,
                         QWidget *parent = 0 );

public slots:
    void switchToOcs( const AmarokAttica::Provider &provider );
    void onOcsFetchStarted();
    void onOcsDataFetched( int err );

signals:
    void switchedToOcs();

private:
    QWidget *m_personsArea;
    QVBoxLayout *m_areaLayout;
    OcsPersonItem::PersonStatus m_status;
    int m_fetchCount;
};

#endif  //AMAROK_OCSPERSONLISTWIDGET_H
