/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_ITUNES_IMPORTER_CONFIG_H
#define AMAROK_ITUNES_IMPORTER_CONFIG_H


#include "databaseimporter/DatabaseImporter.h"

#include <QCheckBox>
#include <QLineEdit>

class QComboBox;
class QLabel;

class ITunesImporterConfig : public DatabaseImporterConfig
{
    Q_OBJECT

    public:
        ITunesImporterConfig( QWidget *parent = 0 );
        virtual ~ITunesImporterConfig() { };

        QString databaseLocation() const { return m_databaseLocationInput->text(); }
    private:
        
        QLabel    *m_databaseLocationLabel;

        QLineEdit *m_databaseLocationInput;
};

#endif
