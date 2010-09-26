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

#include "ui_AmpacheConfigWidget.h"
#include "ui_NewServerWidget.h"


#include <kdebug.h>
#include <kgenericfactory.h>

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

K_PLUGIN_FACTORY( AmpacheSettingsFactory, registerPlugin<AmpacheSettings>(); )
K_EXPORT_PLUGIN( AmpacheSettingsFactory( "kcm_amarok_ampache" ) )


AmpacheSettings::AmpacheSettings(QWidget * parent, const QVariantList & args)
    : KCModule( AmpacheSettingsFactory::componentData(), parent, args )
    , m_lastRowEdited(-1)
    , m_lastColumnEdited(-1)
{
    kDebug( 14310 ) << "Creating Ampache config object";

    //QVBoxLayout* l = new QVBoxLayout( this );
    //QWidget *w = new QWidget;
    m_configDialog = new Ui::AmpacheConfigWidget;
    m_configDialog->setupUi( this );
    m_configDialog->serverList->setMinimumWidth(700);
    m_configDialog->serverList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_configDialog->serverList->verticalHeader()->hide();
    //l->addWidget( w );

    connect ( m_configDialog->serverList, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onCellDoubleClicked(int,int)));
    connect ( m_configDialog->serverList, SIGNAL(cellChanged(int,int)), this, SLOT(saveCellEdit(int,int)));
    connect ( m_configDialog->addButton, SIGNAL( clicked() ), this, SLOT( add() ) );
    connect ( m_configDialog->removeButton, SIGNAL( clicked() ), this, SLOT( remove() ) );
    //connect ( m_configDialog->modifyButton, SIGNAL( clicked() ), this, SLOT( modify() ) );
    connect ( m_configDialog->serverList, SIGNAL ( currentTextChanged ( const QString & ) ), this, SLOT( selectedItemChanged( const QString & ) ) );
    //connect ( m_configDialog->nameEdit, SIGNAL( textChanged ( const QString & )), this,SLOT(serverNameChanged( const QString & )));
}

AmpacheSettings::~AmpacheSettings()
{
}

void
AmpacheSettings::serverNameChanged(const QString & text)
{
   m_configDialog->addButton->setEnabled( !text.isEmpty() );
}

void
AmpacheSettings::save()
{
    kDebug( 14310 ) << "save";
    m_config.save();
    KCModule::save();
}

void
AmpacheSettings::load()
{
    kDebug( 14310 ) << Q_FUNC_INFO;
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

        kDebug( 14310 ) << "adding item" << entry.name;
        serverList->setItem(i, 0, new QTableWidgetItem(entry.name));
        serverList->setItem(i, 1, new QTableWidgetItem(entry.url));
        serverList->setItem(i, 2, new QTableWidgetItem(entry.username));
        QString starPassword = entry.password;
        starPassword.fill('*');
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
    kDebug( 14310 ) << "defaults";
}

void
AmpacheSettings::add()
{
    kDebug( 14310 ) << Q_FUNC_INFO;

    Ui::NewServerWidget newServer;
    QWidget* w = new QWidget();
    newServer.setupUi(w);
    KDialog dialog;
    dialog.setMainWidget(w);
    if(dialog.exec() == QDialog::Accepted)
    {
        AmpacheServerEntry server;
        server.name = newServer.nameLineEdit->text();
        server.url = newServer.serverAddressLineEdit->text();
        server.username = newServer.userNameLineEdit->text();
        server.password = newServer.passwordLineEdit->text();
        if( server.name.isEmpty())
            return;
        m_config.addServer( server );
    }
    loadList();
    emit changed( true );
}

void
AmpacheSettings::remove()
{
    int index = m_configDialog->serverList->currentRow();
    m_configDialog->serverList->removeRow( index );
    m_config.removeServer( index );

    emit changed( true );
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
    kDebug( 14310 ) << Q_FUNC_INFO << row << column;
    QString newValue = m_configDialog->serverList->item(row, column)->text();
    AmpacheServerEntry server = m_config.servers().at(row);
    switch(column)
    {
        case 0:
            server.name = newValue;
            break;
        case 1:
            server.url = newValue;
            break;
        case 2:
            server.name = newValue;
            break;
        case 3:
            server.password = newValue;
            break;
        default:
            qWarning() << Q_FUNC_INFO << "invalid column";
        
    }
    m_config.updateServer(row, server);
    m_configDialog->serverList->resizeColumnToContents(column);
    emit changed( true );
}

void
AmpacheSettings::modify()
{
/*    int index = m_configDialog->serverList->currentRow();

    AmpacheServerEntry server;
    server.name = m_configDialog->nameEdit->text();
    server.url = m_configDialog->serverEdit->text();
    server.username = m_configDialog->userEdit->text();
    server.password = m_configDialog->passEdit->text();
    m_config.updateServer( index, server );
    m_configDialog->serverList->takeItem( index );
    m_configDialog->serverList->insertItem( index, server.name );

    emit changed( true ); */
}

#include "AmpacheSettings.moc"

