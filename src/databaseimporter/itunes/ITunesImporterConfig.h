/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
* copyright              : (C) 2008 Leo Franchi <lfranchi@kde.org>             *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

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
