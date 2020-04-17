
/****************************************************************************************
 * Copyright (c) 2009 John Atkinson <john@fauxnetic.co.uk>                              *
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

#ifndef DATABASECONFIG_H
#define DATABASECONFIG_H

#include "ui_DatabaseConfig.h"
#include "configdialog/ConfigDialogBase.h"

class KConfigDialogManager;
class KConfigSkeleton;

class DatabaseConfig : public ConfigDialogBase, public Ui_DatabaseConfig
{
    Q_OBJECT

    public:
        DatabaseConfig( Amarok2ConfigDialog* parent, KConfigSkeleton *config );
        ~DatabaseConfig() override;

        bool hasChanged() override;
        bool isDefault() override;
        void updateSettings() override;

    public Q_SLOTS:
        void toggleExternalConfigAvailable( int checkBoxState );
        void testDatabaseConnection();

    private Q_SLOTS:
        void updateSQLQuery();

    private:
        /** Returns true if the configuration is complete.
         *
         *  Complete menas that the database, user and host are filled out.
         */
        bool isSQLInfoPresent() const;

        KConfigDialogManager* m_configManager;

};


#endif


