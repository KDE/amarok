/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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
 
#include "ScriptSelector.h"

#include <core/support/Amarok.h>
#include "core/support/Debug.h"

#include <KCategorizedView>
#include <KLocalizedString>
#include <KPluginMetaData>

#include <QLineEdit>
#include <QScrollBar>
#include <QSortFilterProxyModel>

#include <algorithm>

// uber-hacky, this whole thing, make our own script selector?
ScriptSelector::ScriptSelector( QWidget * parent )
    : KPluginWidget( parent )
    , m_scriptCount( 0 )
{
    m_lineEdit = this->findChild<QLineEdit*>();
    if( m_lineEdit )
    {
        m_lineEdit->setPlaceholderText( i18n( "Search Scripts" ) );
        connect( m_lineEdit, &QLineEdit::textChanged, this, &ScriptSelector::slotFiltered );
    }

    m_listView = this->findChild<KCategorizedView*>();
}

ScriptSelector::~ScriptSelector()
{}

void
ScriptSelector::setVerticalPosition( int position )
{
    m_listView->verticalScrollBar()->setSliderPosition( position );
}

int
ScriptSelector::verticalPosition()
{
    return m_listView->verticalScrollBar()->sliderPosition();
}

QString
ScriptSelector::filter()
{
    return m_lineEdit->text();
}

void
ScriptSelector::setFilter( const QString &filter )
{
    m_lineEdit->setText( filter );
}

void
ScriptSelector::addScripts( QVector<KPluginMetaData> pluginInfoList,
                            const QString &categoryName )
{
    DEBUG_BLOCK

    std::sort( pluginInfoList.begin(), pluginInfoList.end()
         , []( const KPluginMetaData &left, const KPluginMetaData &right ){ return left.name() < right.name(); } );
    setConfig( Amarok::config( QStringLiteral("Scripts") ) );
    KPluginWidget::addPlugins( pluginInfoList, categoryName );
    for( const KPluginMetaData &plugin : pluginInfoList )
    {
        m_scriptCount++;
        m_scripts[m_scriptCount] = plugin.pluginId();
    }
}

QString
ScriptSelector::currentItem() const
{
    DEBUG_BLOCK

    QItemSelectionModel *selModel = m_listView->selectionModel();
    const QModelIndexList selIndexes = selModel->selectedIndexes();

    if( !selIndexes.isEmpty() )
    {
        QModelIndex currentIndex = dynamic_cast<QSortFilterProxyModel*>(m_listView->model())->mapToSource( selIndexes[0] );
        if( currentIndex.isValid() )
        {
            debug() << "row: " << currentIndex.row() + 1; //the index start from 1
            debug() << "name: "<< m_scripts[currentIndex.row() + 1];
            return m_scripts[currentIndex.row() + 1];
        }
    }

    return QString();
}

void
ScriptSelector::slotFiltered( const QString &filter )
{
    if( filter.isEmpty() )
        Q_EMIT filtered( false );
    else
        Q_EMIT filtered( true );
}

