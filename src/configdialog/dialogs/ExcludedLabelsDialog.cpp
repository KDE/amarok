/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "ExcludedLabelsDialog.h"

#include "statsyncing/Config.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>
#include <KLineEdit>

#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolButton>

ExcludedLabelsDialog::ExcludedLabelsDialog( StatSyncing::Config *config, QWidget *parent,
                                            Qt::WFlags flags )
    : KDialog( parent, flags )
    , m_statSyncingConfig( config )
    , m_layout( new QGridLayout( mainWidget() ) )
    , m_addLabelLine( new KLineEdit() )
    , m_listWidget( new QListWidget() )
{
    Q_ASSERT( m_statSyncingConfig );
    setButtons( Ok | Cancel );
    setCaption( i18n( "Excluded Labels" ) );

    QLabel *label = new QLabel( i18n( "Selected labels won't by touched by statistics "
            "synchronization" ) );
    label->setWordWrap( true );
    m_layout->addWidget( label, 0, 0, 1, 2 );

    m_addLabelLine->setClickMessage( i18n( "Add Label to Exclude" ) );
    m_layout->addWidget( m_addLabelLine, 1, 0 );
    QAbstractButton *addButton = new QToolButton();
    addButton->setIcon( KIcon( "list-add" ) );
    addButton->setToolTip( i18n( "Add this label to the list of excluded labels" ) );
    connect( addButton, SIGNAL(clicked(bool)), SLOT(slotAddExcludedLabel()) );
    m_layout->addWidget( addButton, 1, 1 );

    m_listWidget->setSelectionMode( QAbstractItemView::MultiSelection );
    m_layout->addWidget( m_listWidget, 2, 0, 1, 2 );

    addLabels( config->excludedLabels(), true );
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setQueryType( Collections::QueryMaker::Label );
    qm->setAutoDelete( true );
    connect( qm, SIGNAL(newResultReady(Meta::LabelList)),
             SLOT(slowNewResultReady(Meta::LabelList)) );
    qm->run();

    connect( this, SIGNAL(okClicked()), SLOT(slotSaveToConfig()) );
}

void
ExcludedLabelsDialog::slowNewResultReady( const Meta::LabelList &labels )
{
    foreach( const Meta::LabelPtr &label, labels )
        addLabel( label->name() );
}

void
ExcludedLabelsDialog::slotAddExcludedLabel()
{
    addLabel( m_addLabelLine->text(), true );
    m_addLabelLine->setText( "" );
}

void
ExcludedLabelsDialog::slotSaveToConfig()
{
    QSet<QString> excluded;
    foreach( const QListWidgetItem *item, m_listWidget->selectedItems() )
    {
        excluded.insert( item->text() );
    }
    m_statSyncingConfig->setExcludedLabels( excluded );
}

void
ExcludedLabelsDialog::addLabel( const QString &label, bool selected )
{
    int count = m_listWidget->count();
    for( int i = 0; i <= count; i++ )
    {
        QModelIndex idx;
        if( i == count )
        {
            // reached end of the list
            m_listWidget->addItem( label );
            idx = m_listWidget->model()->index( i, 0 );
        }
        else if( m_listWidget->item( i )->text() == label )
        {
            // already in list
            return;
        }
        else if( QString::localeAwareCompare( m_listWidget->item( i )->text(), label ) > 0 )
        {
            m_listWidget->insertItem( i, label );
            idx = m_listWidget->model()->index( i, 0 );
        }
        // else continue to next iteration

        // just inserted
        if( idx.isValid() && selected )
            m_listWidget->selectionModel()->select( idx, QItemSelectionModel::Select );
        if( idx.isValid() )
            return;
    }
}

void
ExcludedLabelsDialog::addLabels( const QSet<QString> &labels, bool selected )
{
    foreach( const QString &label, labels )
    {
        addLabel( label, selected );
    }
}
