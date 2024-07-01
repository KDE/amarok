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

#include "TreeController.h"

#include "ConstraintNode.h"
#include "TreeModel.h"
#include "PresetModel.h"

#include "core/support/Debug.h"

#include <QObject>
#include <QTreeView>

APG::TreeController::TreeController( TreeModel* m, QTreeView* v, QWidget* p )
        : QObject( p )
        , m_model( m )
        , m_view( v )
{
    DEBUG_BLOCK
}

APG::TreeController::~TreeController()
{
    DEBUG_BLOCK
}

void
APG::TreeController::addGroup() const
{
    QModelIndex newidx = m_model->insertGroup( m_view->currentIndex() );
    m_view->expandAll();
    m_view->setCurrentIndex( newidx );
}

void
APG::TreeController::addConstraint( const QString& constraintName ) const
{
    QModelIndex newidx = m_model->insertConstraint( m_view->currentIndex(), constraintName );
    m_view->setCurrentIndex( newidx );
}

void
APG::TreeController::removeNode() const
{
    bool r = m_model->removeNode( m_view->currentIndex() );
    if ( !r )
        error() << "for some reason, the node could not be removed";

    // force re-selection of root index
    if ( m_view->currentIndex() == QModelIndex() ) {
        debug() << "deleted root index";
        m_view->setCurrentIndex( m_model->index( 0, 0 ) );
    }
}
