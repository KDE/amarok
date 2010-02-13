/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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


#ifndef APPLET_ITEM_MODEL_H
#define APPLET_ITEM_MODEL_H

#include "amarok_export.h"

#include <QStandardItem>
#include <QStandardItemModel>

namespace Context
{

class AppletItemModel;

class AMAROK_EXPORT AppletItem: public QObject, public QStandardItem
{
    Q_OBJECT

public:
    AppletItem( AppletItemModel *model, const QMap< QString, QVariant>& info );

    QString name() const;
    QString pluginName() const;
    int running() const;

    void setRunning( int count );

private:
    AppletItemModel *m_model;
};

class AMAROK_EXPORT AppletItemModel: public QStandardItemModel
{
    Q_OBJECT

public:
    AppletItemModel( QObject *parent = 0 );

private:
    void populateModel();
};


} //namespace Context

#endif
