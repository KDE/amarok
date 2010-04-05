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

#include <QtGui>

APG::PresetEditDialog::PresetEditDialog( PresetPtr p )
            : QDialog( 0 )
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
    connect( ui.constraintTreeView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
             this, SLOT( currentNodeChanged( const QModelIndex& ) ) );
    ui.constraintTreeView->setCurrentIndex( model->index( 0, 0 ) ); // select the visible root constraint
    ui.constraintTreeView->expandAll();

    QSignalMapper* adderMapper = new QSignalMapper( this );
    connect( adderMapper, SIGNAL( mapped( const QString& ) ), this, SLOT( addNode( const QString& ) ) );

    QMenuBar* menuBar_Actions = new QMenuBar();

    QAction* a;
    QMenu* m = new QMenu( i18n("Add new"), this );
    a = m->addAction( QString( i18n("ConstraintGroup") ) );
    connect( a, SIGNAL( triggered( bool ) ), adderMapper, SLOT( map() ) );
    adderMapper->setMapping( a, i18n("ConstraintGroup") );
    foreach( const QString& name, ConstraintFactory::instance()->names() ) {
        a = m->addAction( name );
        connect( a, SIGNAL( triggered( bool ) ), adderMapper, SLOT( map() ) );
        adderMapper->setMapping( a, name );
    }
    menuBar_Actions->addMenu( m );

    a = menuBar_Actions->addAction( i18n("Remove selected") );
    connect( a, SIGNAL( triggered( bool ) ), m_controller, SLOT( removeNode() ) );

    menuBar_Actions->addSeparator();

    a = QWhatsThis::createAction( this );
    a->setIcon( QIcon() );
    menuBar_Actions->addAction( a );
    ui.treeLayout->insertWidget( 0, menuBar_Actions );

    connect( ui.buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( ui.buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

    QMetaObject::connectSlotsByName( this );
}

void
APG::PresetEditDialog::addNode( const QString& name )
{
    debug() << "Adding new" << name;
    if ( name == i18n("ConstraintGroup") ) {
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
        // TODO: connect to update() signal of widget, and use it redraw the label in the treeview
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
