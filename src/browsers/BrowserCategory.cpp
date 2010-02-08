/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "BrowserCategory.h"

#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"

#include "Debug.h"

#include "ToolBar.h"

#include <QVBoxLayout>
#include <QWidget>

BrowserCategory::BrowserCategory( const QString &name, QWidget *parent )
    : KVBox( parent )
    , m_name( name )
    , m_prettyName( QString() )
    , m_shortDescription( QString() )
    , m_longDescription( QString() )
    , m_parentList( 0 )
    , m_breadcrumb( 0 )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setFrameShape( QFrame::NoFrame );

}

BrowserCategory::~BrowserCategory()
{
    delete m_breadcrumb;
}

QString
BrowserCategory::name() const
{
    return m_name;
}

void
BrowserCategory::setPrettyName( const QString & prettyName )
{
    m_prettyName = prettyName;
}

QString
BrowserCategory::prettyName() const
{
    return m_prettyName;
}

void
BrowserCategory::setShortDescription( const QString &shortDescription )
{
    m_shortDescription = shortDescription;
}

QString
BrowserCategory::shortDescription() const
{
    return m_shortDescription;
}

void
BrowserCategory::setLongDescription( const QString &longDescription )
{
    m_longDescription = longDescription;
}

QString
BrowserCategory::longDescription() const
{
    return m_longDescription;
}

void
BrowserCategory::setIcon( const QIcon & icon )
{
    m_icon = icon;
}

QIcon
BrowserCategory::icon() const
{
    return m_icon;
}

void BrowserCategory::setParentList( BrowserCategoryList * parent )
{
    m_parentList = parent;
}

BrowserCategoryList * BrowserCategory::parentList() const
{
    return m_parentList;
}

void BrowserCategory::activate()
{
    DEBUG_BLOCK
    if ( parentList() )
        parentList()->activate( this );
}

BrowserBreadcrumbItem * BrowserCategory::breadcrumb()
{
    if ( m_breadcrumb == 0 )
        m_breadcrumb = new BrowserBreadcrumbItem( this );
    return m_breadcrumb;
}

void BrowserCategory::setImagePath( const QString & path )
{
    m_imagePath = path;
}

QString BrowserCategory::imagePath()
{
    return m_imagePath;
}

void
BrowserCategory::addAdditionalItem( BrowserBreadcrumbItem * item )
{
    m_additionalItems.append( item );
}

void
BrowserCategory::clearAdditionalItems()
{
    //these are deleted in BrowserBreadcrumbWidget::clearCrumbs
    foreach( BrowserBreadcrumbItem * item, m_additionalItems )
    {
        m_additionalItems.removeAll( item );
        delete item;
    }
}

QList<BrowserBreadcrumbItem *>
BrowserCategory::additionalItems()
{
    return m_additionalItems;
}

#include "BrowserCategory.moc"
