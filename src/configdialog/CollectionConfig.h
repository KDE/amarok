/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONCONFIG_H
#define COLLECTIONCONFIG_H

#include "ui_CollectionConfig.h"
#include "ConfigDialogBase.h"


class CollectionConfig : public ConfigDialogBase, public Ui_CollectionConfig
{
    Q_OBJECT

    public:
        CollectionConfig( QWidget* parent );
        virtual ~CollectionConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();

    private:
        int databaseTypeCode( const QString& type );
};


#endif


