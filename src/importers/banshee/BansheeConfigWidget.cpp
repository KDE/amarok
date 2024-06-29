/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "BansheeConfigWidget.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QDir>

using namespace StatSyncing;

BansheeConfigWidget::BansheeConfigWidget( const QVariantMap &config, QWidget *parent,
                                          Qt::WindowFlags f )
    : SimpleImporterConfigWidget( QStringLiteral("Banshee"), config, parent, f )
{
    const QString defaultPath = QDir::toNativeSeparators(
                QDir::homePath() + QStringLiteral("/.config/banshee-1/banshee.db") );

    KUrlRequester *dbField = new KUrlRequester( QUrl::fromLocalFile(defaultPath) );
    dbField->setFilter( QStringLiteral("banshee.db") );
    addField( QStringLiteral("dbPath"), i18n( "Database location" ), dbField, QStringLiteral("text") );
}

BansheeConfigWidget::~BansheeConfigWidget()
{
}
