/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "FastForwardConfigWidget.h"

#include <QDir>

namespace StatSyncing
{

FastForwardConfigWidget::FastForwardConfigWidget( const QVariantMap &config,
                                                  QWidget *parent, Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    connect( connectionType, SIGNAL(activated(QString)), SLOT(connectionTypeChanged(QString)) );
    embeddedSettings->show();
    externalSettings->hide();

    populateFields();
}

FastForwardConfigWidget::~FastForwardConfigWidget()
{
}

QVariantMap
FastForwardConfigWidget::config() const
{
    QVariantMap cfg = m_config;
    cfg["name"] = targetName->text();

    if( connectionType->currentText() == "SQLite" )
    {
        cfg["dbDriver"] = "QSQLITE";
        cfg["dbPath"] = databaseLocation->text();
    }
    else
    {
        cfg["dbDriver"] =
                connectionType->currentText() == "MySQL" ? "QMYSQL" : "QPSQL";
        cfg["dbName"] = databaseName->text();
        cfg["dbHost"] = hostname->text();
        cfg["dbUser"] = username->text();
        cfg["dbPass"] = password->text();
        cfg["dbPort"] = port->value();
    }

    return cfg;
}

void
FastForwardConfigWidget::connectionTypeChanged( QString connection )
{
    if( connection == "SQLite" )
    {
        embeddedSettings->show();
        externalSettings->hide();
    }
    else
    {
        embeddedSettings->hide();
        externalSettings->show();
    }
}

void
FastForwardConfigWidget::populateFields()
{
    targetName->setText( m_config.value( "name", "Amarok 1.4" ).toString() );

    if( m_config.value( "dbDriver", "QSQLITE" ).toString() == "QSQLITE" )
        connectionType->setCurrentItem( "SQLite" );
    else
        connectionType->setCurrentItem( m_config["dbDriver"].toString() == "QMYSQL"
                ? "MySQL" : "PostgreSQL" );

    const QString defaultPath = QDir::homePath()+"/.kde/share/apps/amarok/collection.db";
    databaseLocation->setText( m_config.value( "dbPath", defaultPath ).toString() );
    databaseName->setText( m_config.value( "dbName", "amarok" ).toString() );
    hostname->setText( m_config.value( "dbHost", "localhost" ).toString() );
    username->setText( m_config.value( "dbUser", "" ).toString() );
    password->setText( m_config.value( "dbPass", "" ).toString() );
    port->setValue( m_config.value( "dbPort", 3306 ).toInt() );
}

} // namespace StatSyncing
