/****************************************************************************************
 * Copyright (c) 2006 Giovanni Venturi <giovanni@kde-it.org>                            *
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

#define DEBUG_PREFIX "EditFilterDialog"

#include "widgets/MetaQueryWidget.h"
#include "EditFilterDialog.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KGlobal>
#include <KLocale>
#include <KMessageBox>

EditFilterDialog::EditFilterDialog( QWidget* parent, const QString &text )
    : KDialog( parent )
    , m_appended( false )
    , m_filterText( text )
{
    setCaption( i18n( "Edit Filter" ) );
    setButtons( User1|User2|Default|Ok|Cancel );
    setDefaultButton( Cancel );
    showButtonSeparator( true );
    m_ui.setupUi( mainWidget() );
    setMinimumSize( minimumSizeHint() );

    // Redefine "Default" button
    KGuiItem defaultButton( i18n("&Append"), "list-add" );
    setButtonWhatsThis( Default, i18n( "<qt><p>By clicking here you can add the defined condition. The \"OK\" button will "
                                        "close the dialog and apply the defined filter. With this button you can add more than "
                                        "one condition to create a more complex filtering condition.</p></qt>" ) );
    setButtonToolTip( Default, i18n( "Add this filter condition to the list" ) );
    setButtonGuiItem( Default, defaultButton );

    // define "User1" button
    KGuiItem user1Button( i18n("&Clear"), "list-remove" );
    setButtonWhatsThis( User1, i18n( "<p>By clicking here you will clear the filter. If you intend to "
                                     "undo the last appending just click on the \"Undo\" button.</p>" ) );
    setButtonToolTip(User1, i18n( "Clear the filter" ) );
    setButtonGuiItem( User1, user1Button );

    // define "User2" button
    KGuiItem user2Button( i18nc("this \"undo\" will undo the last appended filter... be careful how you will translate it "
       "to avoid two buttons (\"Cancel\" and \"Undo\") with same label in the same dialog", "&Undo"), "edit-undo" );
    setButtonWhatsThis( User2, i18n( "<p>Clicking here will remove the last appended filter. "
                "You cannot undo more than one action.</p>" ) );
    setButtonToolTip( User2, i18n( "Remove last appended filter" ) );
    setButtonGuiItem( User2, user2Button );

    // check "select all words" as default
    m_ui.filterActionGroupBox->setEnabled( !MetaQueryWidget::isNumeric( m_ui.attributeQuery->filter().field ) );
    connect( m_ui.attributeQuery, SIGNAL(changed(const MetaQueryWidget::Filter&)), this, SLOT(slotAttributeChanged()) );
    m_ui.matchAll->setChecked( true );

    // you need to append at least one filter condition to specify if do
    // an "AND" or an "OR" with the next condition if the filter is empty
    //
    if( m_filterText.isEmpty() )
    {
        m_ui.andButton->setEnabled( false );
        m_ui.orButton->setEnabled( false );
    }

    // check "AND" condition as default
    m_ui.andButton->setChecked( true );

    connect( this, SIGNAL(okClicked()), this, SLOT(slotOk()) );
    connect( this, SIGNAL(defaultClicked()) , this, SLOT(slotAppend()) );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(slotClear()) );
    connect( this, SIGNAL(user2Clicked()), this, SLOT(slotUndo()) );

}

EditFilterDialog::~EditFilterDialog()
{
}

void EditFilterDialog::slotAttributeChanged()
{
    // only enable the "match all words" radio on fields that can actually
    // have several words.
    m_ui.filterActionGroupBox->setEnabled( !MetaQueryWidget::isNumeric( m_ui.attributeQuery->filter().field ) );
}

void EditFilterDialog::slotAppend()
{
    MetaQueryWidget::Filter filter = m_ui.attributeQuery->filter();

    // now append the filter rule if not empty
    if( filter.condition == MetaQueryWidget::Contains && filter.value.isEmpty() )
    {
        KMessageBox::sorry( 0, i18n("<p>Sorry but the filter rule cannot be set. The text field is empty. "
                    "Please type something into it and retry.</p>"), i18n("Empty Text Field"));
        return;
    }

    if( !m_appended )
    {
        // it's the first rule
        m_appended = true;
        m_ui.andButton->setEnabled( true );
        m_ui.orButton->setEnabled( true );
    }

    m_previousFilterText = m_filterText;

    if( filter.field == 0 /*simple search*/ )
    {
        debug() << "selected text: '" << filter.value << "'";

        QStringList list = filter.value.split( ' ' );
        for ( QStringList::ConstIterator it = list.constBegin(), end = list.constEnd(); it != end; ++it )
        {
            QString word = *it;

            // exactly the words
            if( m_ui.matchLiteral->isChecked() )
                word = "\"" + word + "\"";

            // exclude words
            if( m_ui.matchNot->isChecked() )
                word = "-" + word;

            // at least one word
            if( m_ui.matchAny->isChecked() && !m_filterText.isEmpty() )
                word = "OR " + word;

            m_filterText += " " + word;
        }
    }
    else
    {
        if( !m_filterText.isEmpty() )
        {
            m_filterText += ' ';
            if( m_ui.orButton->isChecked() )
                m_filterText += "OR ";
        }

        m_filterText += filter.toString( m_ui.matchNot->isChecked() );
    }
    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotClear()
{
    m_previousFilterText = m_filterText;
    m_filterText = "";

    // no filter appended cause all cleared
    m_appended = false;
    m_ui.andButton->setEnabled( false );
    m_ui.orButton->setEnabled( false );

    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotUndo()
{
    m_filterText = m_previousFilterText;
    if (m_filterText.isEmpty())
    {
        // no filter appended cause all cleared
        m_appended = false;
        m_ui.andButton->setEnabled( false );
        m_ui.orButton->setEnabled( false );
    }
    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotOk() // SLOT
{
    // If there's a filter typed in but unadded, add it.
    // This makes it easier to just add one condition - you only need to press OK.
    MetaQueryWidget::Filter filter = m_ui.attributeQuery->filter();
    if( filter.condition != MetaQueryWidget::Contains ||
        !filter.value.isEmpty() )
        slotAppend();

    // Don't let OK do anything if they haven't set any filters.
    if (m_appended)
        accept();
}

#include "EditFilterDialog.moc"

