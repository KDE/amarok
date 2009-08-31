/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CONFIGDIALOGBASE_H
#define CONFIGDIALOGBASE_H

#include <QWidget>


class ConfigDialogBase : public QWidget
{
    Q_OBJECT

    signals:
        void settingsChanged( const QString& );

    public:
        virtual void updateSettings() = 0;
        virtual void updateWidgets() {}
        virtual void updateWidgetsDefault() {}

        virtual bool hasChanged() = 0;
        virtual bool isDefault() = 0;

    protected:
        ConfigDialogBase( QWidget* parent ) : QWidget( parent ) {}
};


#endif

