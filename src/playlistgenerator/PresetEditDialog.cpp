/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "APG::PresetEditDialog"

#include "PresetEditDialog.h"

#include "ConstraintNode.h"
#include "ConstraintFactory.h"
#include "TreeController.h"
#include "TreeModel.h"

#include "core/support/Debug.h"

#include <QMenu>
#include <QMenuBar>
#include <QSignalMapper>
#include <QWhatsThis>

APG::PresetEditDialog::PresetEditDialog( const PresetPtr &p )
            : QDialog( nullptr )
            , m_preset( p )
{
    DEBUG_BLOCK

    ui.setupUi( this );

    TreeModel* model = new TreeModel( m_preset->constraintTreeRoot(), this );
    m_controller = new TreeController( model, ui.constraintTreeView, this );

    ui.lineEdit_Title->setText( m_preset->title() );

    ui.constraintTreeView->setModel( model );
    ui.constraintTreeView->setSelectionMode( QAbstractItemView::SingleSelection );
    ui.constraintTreeView->setHeaderHidden( true );
    connect( ui.constraintTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &PresetEditDialog::currentNodeChanged );
    ui.constraintTreeView->setCurrentIndex( model->index( 0, 0 ) ); // select the visible root constraint
    ui.constraintTreeView->expandAll();

    QSignalMapper* adderMapper = new QSignalMapper( this );
    connect( adderMapper, QOverload<const QString&>::of(&QSignalMapper::mappedString),
             this, &PresetEditDialog::addNode );

    QMenuBar* menuBar_Actions = new QMenuBar( this );
    menuBar_Actions->setNativeMenuBar( false );  //required to make the menu work on OS X

    QAction* a;
    QMenu* m = new QMenu( i18n("Add new"), this );
    a = m->addAction( i18n("Constraint Group") );
    connect( a, &QAction::triggered, adderMapper, QOverload<>::of(&QSignalMapper::map) );
    adderMapper->setMapping( a, i18n("Constraint Group") );
    for( const QString& name : ConstraintFactory::instance()->i18nNames() ) {
        a = m->addAction( name );
        connect( a, &QAction::triggered, adderMapper, QOverload<>::of(&QSignalMapper::map) );
        adderMapper->setMapping( a, name );
    }
    menuBar_Actions->addMenu( m );

    a = menuBar_Actions->addAction( i18n("Remove selected") );
    connect( a, &QAction::triggered, m_controller, &APG::TreeController::removeNode );

    menuBar_Actions->addSeparator();

    a = QWhatsThis::createAction( this );
    a->setIcon( QIcon() );
    menuBar_Actions->addAction( a );
    ui.treeLayout->insertWidget( 0, menuBar_Actions );

    connect( ui.buttonBox, &QDialogButtonBox::accepted, this, &APG::PresetEditDialog::accept );
    connect( ui.buttonBox, &QDialogButtonBox::rejected, this, &APG::PresetEditDialog::reject );

    QMetaObject::connectSlotsByName( this );
}

void
APG::PresetEditDialog::addNode( const QString& name )
{
    debug() << "Adding new" << name;
    if ( name == i18n("Constraint Group") ) {
        m_controller->addGroup();
    } else {
        m_controller->addConstraint( name );
    }
}

void
APG::PresetEditDialog::removeNode()
{
    debug() << "Removing selected node";
    m_controller->removeNode();
}

void
APG::PresetEditDialog::currentNodeChanged( const QModelIndex& index )
{
    if( index.isValid() )
    {
        ConstraintNode* n = static_cast<ConstraintNode*>( index.internalPointer() );
        if ( !m_widgetStackPages.contains( n ) ) {
            debug() << "Inserting new constraint edit widget into the stack";
            QWidget* w = n->editWidget();
            m_widgetStackPages.insert( n, ui.stackedWidget_Editors->addWidget( w ) );
        }
        ui.stackedWidget_Editors->setCurrentIndex( m_widgetStackPages.value( n ) );
    }
}

void
APG::PresetEditDialog::accept()
{
    QDialog::accept();
}

void
APG::PresetEditDialog::reject()
{
    QDialog::reject();
}

void
APG::PresetEditDialog::on_lineEdit_Title_textChanged( const QString& t )
{
    m_preset->setTitle( t );
}
