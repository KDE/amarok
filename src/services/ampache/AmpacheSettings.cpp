/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "AmpacheSettings.h"

#include "AddServerDialog.h"
#include "ui_AmpacheConfigWidget.h"

#include <QTableWidget>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON( ampachesettings, "amarok_service_ampache_config.json", registerPlugin<AmpacheSettings>(); )

AmpacheSettings::AmpacheSettings( QWidget *parent, const QVariantList &args )
    : KCModule( parent, args )
    , m_configDialog(new Ui::AmpacheConfigWidget)
    , m_lastRowEdited(-1)
    , m_lastColumnEdited(-1)
{
    m_configDialog->setupUi( this );
    m_configDialog->serverList->setMinimumWidth(700);
    m_configDialog->serverList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_configDialog->serverList->verticalHeader()->hide();

    connect ( m_configDialog->serverList, &QTableWidget::cellDoubleClicked, this, &AmpacheSettings::onCellDoubleClicked );
    connect ( m_configDialog->serverList, &QTableWidget::cellChanged, this, &AmpacheSettings::saveCellEdit );
    connect ( m_configDialog->addButton, &QPushButton::clicked, this, &AmpacheSettings::add );
    connect ( m_configDialog->removeButton, &QPushButton::clicked, this, &AmpacheSettings::remove );
}

AmpacheSettings::~AmpacheSettings()
{
    delete m_configDialog;
}

void
AmpacheSettings::serverNameChanged(const QString & text)
{
   m_configDialog->addButton->setEnabled( !text.isEmpty() );
}

void
AmpacheSettings::save()
{
    m_config.save();
    KCModule::save();
}

void
AmpacheSettings::load()
{
    loadList();
    KCModule::load();
}

void
AmpacheSettings::loadList()
{
    QTableWidget* serverList = m_configDialog->serverList;
    serverList->setRowCount(m_config.servers().size());
    for( int i = 0; i < m_config.servers().size(); i++ )
    {
        AmpacheServerEntry entry = m_config.servers().at( i );

        serverList->setItem(i, 0, new QTableWidgetItem(entry.name));
        serverList->setItem(i, 1, new QTableWidgetItem(entry.url.url()));
        serverList->setItem(i, 2, new QTableWidgetItem(entry.username));
        QString starPassword = entry.password;
        starPassword.fill(QLatin1Char('*'));
        QTableWidgetItem* password = new QTableWidgetItem(starPassword);
        password->setData(0xf00, entry.password);
        serverList->setItem(i, 3, password);
    }
    serverList->resizeColumnsToContents();
    int columnWidth = serverList->columnWidth(3) + serverList->columnViewportPosition(3);
    serverList->setMinimumWidth( qBound( 200, columnWidth, 700) );

}

void
AmpacheSettings::defaults()
{
}

void
AmpacheSettings::add()
{
    AddServerDialog dialog;
    if(dialog.exec() == QDialog::Accepted)
    {
        AmpacheServerEntry server;
        server.name = dialog.name();
        server.url = dialog.url();
        server.username = dialog.username();
        server.password = dialog.password();
        server.addToCollection = false;
        if( server.name.isEmpty())
            return;
        m_config.addServer( server );
    }
    loadList();
    Q_EMIT changed( true );
}

void
AmpacheSettings::remove()
{
    int index = m_configDialog->serverList->currentRow();
    m_configDialog->serverList->removeRow( index );
    m_config.removeServer( index );

    Q_EMIT changed( true );
}

void
AmpacheSettings::onCellDoubleClicked(int row, int column)
{
    QTableWidgetItem* item = m_configDialog->serverList->item(row, column);
    m_configDialog->serverList->editItem(item);
    m_lastRowEdited = row;
    m_lastColumnEdited = column;
}

void
AmpacheSettings::saveCellEdit(int row, int column)
{
    if(m_lastRowEdited != row || m_lastColumnEdited != column) //only worry about user edits
        return;
//     kDebug( 14310 ) << Q_FUNC_INFO << row << column;
    QString newValue = m_configDialog->serverList->item(row, column)->text();
    AmpacheServerEntry server = m_config.servers().at(row);
    switch(column)
    {
        case 0:
            server.name = newValue;
            break;
        case 1:
            server.url = QUrl( newValue );
            break;
        case 2:
            server.username = newValue;
            break;
        case 3:
            server.password = newValue;
            break;
        default:
            qWarning() << Q_FUNC_INFO << "invalid column";
        
    }
    m_config.updateServer(row, server);
    m_configDialog->serverList->resizeColumnToContents(column);
    Q_EMIT changed( true );
}


#include "moc_AmpacheSettings.cpp"
#include "AmpacheSettings.moc"

