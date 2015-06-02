/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "PlaylistSortWidget.h"

#include "core/support/Debug.h"
#include "PlaylistActions.h"
#include "PlaylistModelStack.h"
#include "proxymodels/SortScheme.h"

#include <KConfigGroup>
#include <KStandardDirs>

namespace Playlist
{

SortWidget::SortWidget( QWidget *parent )
    : QWidget( parent )
{
    setFixedHeight( 28 );
    setContentsMargins( 3, 0, 3, 0 );

    m_layout = new QHBoxLayout( this );
    setLayout( m_layout );
    m_layout->setSpacing( 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );

    BreadcrumbItemButton *rootItem = new BreadcrumbItemButton(
            QIcon::fromTheme( QPixmap( KStandardDirs::locate( "data", "amarok/images/playlist-sorting-16.png" ) ) ),
            QString(), this );
    rootItem->setToolTip( i18n( "Clear the playlist sorting configuration." ) );
    m_layout->addWidget( rootItem );
    connect( rootItem, SIGNAL(clicked()), this, SLOT(trimToLevel()) );

    m_ribbon = new QHBoxLayout();
    m_layout->addLayout( m_ribbon );
    m_ribbon->setContentsMargins( 0, 0, 0, 0 );
    m_ribbon->setSpacing( 0 );

    m_addButton = new BreadcrumbAddMenuButton( this );
    m_addButton->setToolTip( i18n( "Add a playlist sorting level." ) );
    m_layout->addWidget( m_addButton );
    m_layout->addStretch( 10 );

    m_urlButton = new BreadcrumbUrlMenuButton( "playlist", this );
    m_layout->addWidget( m_urlButton );

    connect( m_addButton->menu(), SIGNAL(actionClicked(QString)), this, SLOT(addLevel(QString)) );
    connect( m_addButton->menu(), SIGNAL(shuffleActionClicked()), The::playlistActions(), SLOT(shuffle()) );

    QString sortPath = Amarok::config( "Playlist Sorting" ).readEntry( "SortPath", QString() );
    readSortPath( sortPath );
}

SortWidget::~SortWidget()
{}

void
SortWidget::addLevel( QString internalColumnName, Qt::SortOrder sortOrder )  //private slot
{
    BreadcrumbLevel *bLevel = new BreadcrumbLevel( internalColumnName );
    BreadcrumbItem *item = new BreadcrumbItem( bLevel, this );
    m_ribbon->addWidget( item );
    connect( item, SIGNAL(clicked()), this, SLOT(onItemClicked()) );
    connect( item->menu(), SIGNAL(actionClicked(QString)), this, SLOT(onItemSiblingClicked(QString)) );
    connect( item->menu(), SIGNAL(shuffleActionClicked()), this, SLOT(onShuffleSiblingClicked()) );
    connect( item, SIGNAL(orderInverted()), this, SLOT(updateSortScheme()) );
    if( sortOrder != item->sortOrder() )
        item->invertOrder();
    m_addButton->updateMenu( levels() );
    updateSortScheme();
}

void
SortWidget::trimToLevel( const int level )
{
    for( int i = m_ribbon->count() - 1 ; i > level; i-- )
    {
        BreadcrumbItem *item = qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() );
        m_ribbon->removeWidget( item );
        item->deleteLater();
    }
    updateSortScheme();
    m_addButton->updateMenu( levels() );
}

QStringList
SortWidget::levels() const
{
    QStringList levels = QStringList();
    for( int i = 0; i < m_ribbon->count(); ++i )
        levels << qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->name();
    return levels;
}

void
SortWidget::onItemClicked()
{
    const int level = m_ribbon->indexOf( qobject_cast< QWidget * >( sender()->parent() ) );
    trimToLevel( level );
}

void
SortWidget::onItemSiblingClicked( QString internalColumnName )
{
    const int level = m_ribbon->indexOf( qobject_cast< QWidget * >( sender()->parent() ) );
    trimToLevel( level - 1 );
    addLevel( internalColumnName );
}

void
SortWidget::onShuffleSiblingClicked()
{
    const int level = m_ribbon->indexOf( qobject_cast< QWidget * >( sender()->parent() ) );
    trimToLevel( level - 1 );
    The::playlistActions()->shuffle();
}

void
SortWidget::updateSortScheme()
{
    SortScheme scheme = SortScheme();
    for( int i = 0; i < m_ribbon->count(); ++i )    //could be faster if done with iterator
    {
        QString name( qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->name() );
        Column category = columnForName( name );
        Qt::SortOrder sortOrder = qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->sortOrder();
        scheme.addLevel( SortLevel( category, sortOrder ) );
    }
    ModelStack::instance()->sortProxy()->updateSortMap( scheme );

    KConfigGroup config = Amarok::config( "Playlist Sorting" );
    config.writeEntry( "SortPath", sortPath() );
}

QString
SortWidget::sortPath() const
{
    QString path;
    for( int i = 0; i < m_ribbon->count(); ++i )    //could be faster if done with iterator
    {
        QString name( qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->name() );
        Qt::SortOrder sortOrder = qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->sortOrder();
        QString level = name + '_' + ( sortOrder ? "des" : "asc" );
        path.append( ( i == m_ribbon->count() - 1 ) ? level : ( level + '-' ) );
    }
    return path;
}

void
SortWidget::readSortPath( const QString &sortPath )
{
    trimToLevel();

    QStringList levels = sortPath.split( '-' );
    foreach( const QString &level, levels )
    {
        QStringList levelParts = level.split( '_' );
    /*
     * Check whether the configuration is valid. If indexOf
     * returns -1, the entry is corrupted. We can't use columnForName
     * here, as it will do a static_cast, which is UB when indexOf is -1
     * as there's no corresponding enum value
     * (C++ standard 5.2.9 Static cast [expr.static.cast] paragraph 7)
     */
        if( levelParts.count() > 2
        || ( Playlist::PlaylistColumnInfos::internalNames().
                           indexOf( levelParts.value(0) ) == -1) )
            warning() << "Playlist sorting load error: Invalid sort level " << level;
        else if( levelParts.value( 1 ) == QString( "asc" ) )
            addLevel( levelParts.value( 0 ), Qt::AscendingOrder );
        else if( levelParts.value( 1 ) == QString( "des" ) )
            addLevel( levelParts.value( 0 ), Qt::DescendingOrder );
        else
            warning() << "Playlist sorting load error: Invalid sort order for level " << level;
    }
}

QString
SortWidget::prettySortPath() const
{
    QString prettyPath;
    for( int i = 0; i < m_ribbon->count(); ++i )    //could be faster if done with iterator
    {
        QString name( qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->name() );
        QString prettyName( qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->prettyName() );
        Qt::SortOrder sortOrder = qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() )->sortOrder();
        QString prettyLevel = prettyName + ( sortOrder ? "↓" : "↑" );
        prettyPath.append( ( i == m_ribbon->count() - 1 ) ? prettyLevel : ( prettyLevel + " > " ) );
        //TODO: see how this behaves on RTL systems
    }
    return prettyPath;
}

}
