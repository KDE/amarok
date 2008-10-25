/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/


#ifndef AMAROK_FASTFORWARD_IMPORTER_CONFIG_H
#define AMAROK_FASTFORWARD_IMPORTER_CONFIG_H

#include "databaseimporter/DatabaseImporter.h"

#include <QLineEdit>

class QCheckBox;
class QComboBox;
class QLabel;

class FastForwardImporterConfig : public DatabaseImporterConfig
{
    Q_OBJECT

    public:
        FastForwardImporterConfig( QWidget *parent = 0 );
        virtual ~FastForwardImporterConfig() { };

        FastForwardImporter::ConnectionType connectionType() const;
        QString databaseLocation() const { return m_databaseLocationInput->text(); }
        QString databaseName() const { return m_databaseInput->text(); }
        QString databaseHost() const { return m_hostnameInput->text(); }
        QString databaseUser() const { return m_usernameInput->text(); }
        QString databasePass() const { return m_passwordInput->text(); }

    private slots:
        void connectionChanged( int index );

    private:
        QCheckBox *m_importArtworkCheck;
        QComboBox *m_connectionCombo;

        QLabel    *m_databaseLocationLabel;
        QLabel    *m_databaseLabel;
        QLabel    *m_hostnameLabel;
        QLabel    *m_usernameLabel;
        QLabel    *m_passwordLabel;

        QLineEdit *m_databaseLocationInput;
        QLineEdit *m_databaseInput;
        QLineEdit *m_hostnameInput;
        QLineEdit *m_usernameInput;
        QLineEdit *m_passwordInput;
};

#endif
