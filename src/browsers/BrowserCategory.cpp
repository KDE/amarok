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

#include "App.h"
#include "amarokconfig.h"
#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"
#include "PaletteHandler.h"
#include "core/support/Debug.h"


BrowserCategory::BrowserCategory( const QString &name, QWidget *parent )
    : BoxWidget( true, parent )
    , m_name( name )
    , m_parentList( nullptr )
{
    setObjectName( name );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    connect( pApp, &App::settingsChanged, this, &BrowserCategory::slotSettingsChanged );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &BrowserCategory::slotSettingsChanged );
}

BrowserCategory::~BrowserCategory()
{
}

QString
BrowserCategory::name() const
{
    return m_name;
}

void
BrowserCategory::setPrettyName( const QString &prettyName )
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

void
BrowserCategory::setBackgroundImage(const QString& path)
{
    if ( path.isEmpty() || !QUrl(path).isLocalFile() ) {
        setStyleSheet( QString() );
        return;
    }

    // Hack alert: Use the class name of the most derived object (using polymorphism) for CSS
    // This is required to limit the style to this specific class only (avoiding cascading)
    // \sa http://doc.qt.nokia.com/latest/stylesheet-syntax.html#widgets-inside-c-namespaces
    const QString escapedClassName = QString( QLatin1String( metaObject()->className() ) ).replace( QLatin1String("::"), QLatin1String("--") );
    setStyleSheet( QStringLiteral("%1 { background-image: url(\"%2\"); \
            background-repeat: no-repeat; \
            background-attachment: fixed; \
            background-position: center; }").arg( escapedClassName, path )
    );
}

void BrowserCategory::slotSettingsChanged()
{
    setBackgroundImage( AmarokConfig::showBrowserBackgroundImage() ? m_imagePath : QString() );
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
        parentList()->setActiveCategory( this );
}

BrowserBreadcrumbItem *BrowserCategory::breadcrumb()
{
    return new BrowserBreadcrumbItem( this );
}

void BrowserCategory::setImagePath( const QString & path )
{
    m_imagePath = path;
}

QString BrowserCategory::imagePath() const
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
    const auto items = m_additionalItems;
    for( BrowserBreadcrumbItem *item : items)
    {
        m_additionalItems.removeAll( item );
        /* deleting immediately isn't safe, this method may be called from an inner
         * QEventLoop inside QMenu::exec() of another breadcrumb item, which could
         * then leas to crash bug 265626 */
        item->deleteLater();
    }
}

QList<BrowserBreadcrumbItem *>
BrowserCategory::additionalItems()
{
    return m_additionalItems;
}

