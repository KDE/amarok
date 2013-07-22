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
#include <QLayout>

using namespace StatSyncing;

FastForwardConfigWidget::FastForwardConfigWidget( const QVariantMap &config,
                                                  QWidget *parent, Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    m_embeddedDbSettings << m_databaseLocation << m_databaseLocationLabel;
    m_externalDbSettings << m_databaseName << m_databaseNameLabel << m_hostname
                         << m_hostnameLabel << m_password << m_passwordLabel
                         << m_port << m_portLabel << m_username << m_usernameLabel;

    connect( m_connectionType, SIGNAL(activated(QString)),
                                                  SLOT(connectionTypeChanged(QString)) );
    populateFields();

    // Manually hide fields at the beginning
    connectionTypeChanged( m_connectionType->currentText() );
}

FastForwardConfigWidget::~FastForwardConfigWidget()
{
}

QVariantMap
FastForwardConfigWidget::config() const
{
    QVariantMap cfg = m_config;
    cfg["name"] = m_targetName->text();

    if( m_connectionType->currentText() == "SQLite" )
    {
        cfg["dbDriver"] = "QSQLITE";
        cfg["dbPath"] = m_databaseLocation->text();
    }
    else
    {
        cfg["dbDriver"] =
                m_connectionType->currentText() == "MySQL" ? "QMYSQL" : "QPSQL";
        cfg["dbName"] = m_databaseName->text();
        cfg["dbHost"] = m_hostname->text();
        cfg["dbUser"] = m_username->text();
        cfg["dbPass"] = m_password->text();
        cfg["dbPort"] = m_port->value();
    }

    return cfg;
}

void
FastForwardConfigWidget::connectionTypeChanged( const QString &connection )
{
    const bool embedded = connection == "SQLite";

    foreach( QWidget *widget, m_embeddedDbSettings )
        widget->setVisible( embedded );

    foreach( QWidget *widget, m_externalDbSettings )
        widget->setVisible( !embedded );
}

void
FastForwardConfigWidget::populateFields()
{
    m_targetName->setText( m_config.value( "name", "Amarok 1.4" ).toString() );

    if( m_config.value( "dbDriver", "QSQLITE" ).toString() == "QSQLITE" )
        m_connectionType->setCurrentItem( "SQLite" );
    else
        m_connectionType->setCurrentItem(
                  m_config["dbDriver"].toString() == "QMYSQL" ? "MySQL" : "PostgreSQL" );

    const QString defaultPath = QDir::homePath()+"/.kde/share/apps/amarok/collection.db";
    m_databaseLocation->setText( m_config.value( "dbPath", defaultPath ).toString() );
    m_databaseName->setText( m_config.value( "dbName", "amarokdb" ).toString() );
    m_hostname->setText( m_config.value( "dbHost", "localhost" ).toString() );
    m_username->setText( m_config.value( "dbUser", "amarokuser" ).toString() );
    m_password->setText( m_config.value( "dbPass", "" ).toString() );
    m_port->setValue( m_config.value( "dbPort", 3306 ).toInt() );
}
