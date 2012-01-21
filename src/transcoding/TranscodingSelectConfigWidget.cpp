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

#include "TranscodingSelectConfigWidget.h"

#include <KIcon>
#include <KLocalizedString>

using namespace Transcoding;

void
SelectConfigWidget::fillInChoices( const Configuration &savedConfiguration )
{
    clear();
    if( savedConfiguration.isValid() && !savedConfiguration.isJustCopy() )
        addItem( KIcon( "audio-x-generic" ), i18nc( "An option in combo box to always transcode; "
            "%1 are transcoding options", "Always (%1)", savedConfiguration.prettyName() ),
            DontChange );

    if( !savedConfiguration.isValid() )
        addItem( KIcon( "view-choose" ), i18n( "Ask before each transfer" ), DontChange );
    else
        addItem( KIcon( "view-choose" ), i18n( "Ask before each transfer" ), Forget );

    if( savedConfiguration.isValid() && savedConfiguration.isJustCopy() )
    {
        addItem( KIcon( "edit-copy" ), i18n( "Never" ), DontChange );
        setCurrentIndex( count() - 1 );
    }
    else
        addItem( KIcon( "edit-copy" ), i18n( "Never" ), JustCopy );
}

SelectConfigWidget::Choice
SelectConfigWidget::currentChoice() const
{
    if( currentIndex() < 0 )
        return DontChange; // nothing better to return
    return Choice( itemData( currentIndex() ).toInt() );
}

#include "TranscodingSelectConfigWidget.moc"
