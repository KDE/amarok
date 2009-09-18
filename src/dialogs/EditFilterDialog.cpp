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

#include "EditFilterDialog.h"

#include "amarokconfig.h"
#include "Debug.h"
#include "collection/CollectionManager.h"
#include "collection/MetaQueryMaker.h"

#include <KGlobal>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>

#include <QLabel>
#include <QLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>


EditFilterDialog::EditFilterDialog( QWidget* parent, const QString &text )
    : KDialog( parent )
    , m_appended( false )
    , m_filterText( text )
{
    setCaption( i18n( "Edit Filter" ) );
    setModal( true );
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
    
    //setMainWidget( m_ui );
        
    m_vector.push_back( "Simple Search" );
    m_vector.push_back( "album" );
    m_vector.push_back( "artist" );
    m_vector.push_back( "composer" );
    m_vector.push_back( "genre" );
    m_vector.push_back( "playcount" );
    m_vector.push_back( "score" );
    m_vector.push_back( "title" );
    m_vector.push_back( "track" );
    m_vector.push_back( "year" );

    // the "Simple Search" text is selected in the comboKeyword
    m_selectedIndex = 0;
    
    
    connect( m_ui.keywordCombo, SIGNAL( activated( int ) ), SLOT(selectedKeyword( int ) ) );

    connect( m_ui.minimum1, SIGNAL( valueChanged( int ) ), SLOT(minSpinChanged( int ) ) );
    connect( m_ui.maximum1, SIGNAL( valueChanged( int ) ), SLOT(maxSpinChanged( int ) ) );

    // type text selected
    textWanted();

    // check the "One Value Choosing" by default
    chooseOneValue();

    connect( m_ui.conditionCombo, SIGNAL(activated( int ) ), SLOT(chooseCondition( int ) ) );

    m_checkActions << m_ui.matchAll;
    m_checkActions << m_ui.matchAny;
    m_checkActions << m_ui.matchLiteral;
    m_checkActions << m_ui.matchNot;

    connect( m_ui.matchAll,     SIGNAL( clicked() ), SLOT( slotCheckAll() ) );
    connect( m_ui.matchAny,     SIGNAL( clicked() ), SLOT( slotCheckAtLeastOne() ) );
    connect( m_ui.matchLiteral, SIGNAL( clicked() ), SLOT( slotCheckExactly() ) );
    connect( m_ui.matchNot,     SIGNAL( clicked() ), SLOT( slotCheckExclude() ) );

    // check "select all words" as default
    slotCheckAll();

    m_ui.invertButton->setEnabled( false );

    connect( m_ui.invertButton, SIGNAL( clicked() ), SLOT( assignPrefixNOT() ) );

    // you need to append at least one filter condition to specify if do
    // an "AND" or an "OR" with the next condition if the filter is empty
    if( m_filterText.isEmpty() )
      m_ui.groupBox_3->setEnabled( false );

    connect( m_ui.andButton, SIGNAL(clicked()), SLOT(slotCheckAND()) );
    connect( m_ui.orButton, SIGNAL(clicked()), SLOT(slotCheckOR()) );

    // check "AND" condition as default
    slotCheckAND();

    connect( this, SIGNAL(okClicked()), this, SLOT(slotOk() ) );
    connect( this, SIGNAL( defaultClicked() ) , this, SLOT(slotDefault() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
    connect( this, SIGNAL( user2Clicked() ), this, SLOT( slotUser2() ) );
    
    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    if( !coll )
        return;

    QueryMaker *artist = coll->queryMaker()->setQueryType( QueryMaker::Artist );
    QueryMaker *album = coll->queryMaker()->setQueryType( QueryMaker::Album );
    QueryMaker *composer = coll->queryMaker()->setQueryType( QueryMaker::Composer );
    QueryMaker *genre = coll->queryMaker()->setQueryType( QueryMaker::Genre );
    QList<QueryMaker*> queries;
    queries << artist << album << composer << genre;

    //MetaQueryMaker will run multiple different queries just fine as long as we do not use it
    //to set the query type. Configuring the queries is ok though

    MetaQueryMaker *dataQueryMaker = new MetaQueryMaker( queries );
    connect( dataQueryMaker, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), SLOT( resultReady( QString, Meta::ArtistList ) ), Qt::QueuedConnection );
    connect( dataQueryMaker, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
    connect( dataQueryMaker, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), SLOT( resultReady( QString, Meta::ComposerList ) ), Qt::QueuedConnection );
    connect( dataQueryMaker, SIGNAL( newResultReady( QString, Meta::GenreList ) ), SLOT( resultReady( QString, Meta::GenreList ) ), Qt::QueuedConnection );
    dataQueryMaker->run();
    
}

EditFilterDialog::~EditFilterDialog()
{
    delete m_ui.editKeywordBox;
}

QString EditFilterDialog::filter() const
{
    return m_filterText;
}

void EditFilterDialog::exclusiveSelectOf( int which )
{
    int size = m_checkActions.count();

    for( int i = 0; i < size; i++ )
    {
        if ( i != which )
            m_checkActions[i]->setChecked( false );
        else
            m_checkActions[i]->setChecked( true );
    }
}

QString EditFilterDialog::keywordConditionString( const QString& keyword ) const
{
    // this member is called when there is a keyword that needs numeric attributes
    QString result, unit;

    switch(m_ui.conditionCombo->currentIndex())
    {
        case 0:
            // less than...
            result = m_strPrefixNOT + keyword + ":<";
            //if (keyword == "length")
            //    result += QString::number( m_ui.minimum1->value() * 60 + m_ui.minimum2->value() ) + unit;
            //else
                result += m_ui.minimum1->text() + unit;
            break;
        case 1:
            // greater than...
            result = m_strPrefixNOT + keyword + ":>";
            //if (keyword == "length")
                //result += QString::number( m_ui.minimum1->value() * 60 + m_ui.minimum2->value() ) + unit;
            //else
                result += m_ui.minimum1->text() + unit;
            break;
        case 2:
            // equal to...
            //if (keyword == "length")
                //result = m_strPrefixNOT + "length:" + QString::number( m_ui.minimum1->value() * 60
                        //+ m_ui.minimum2->value() ) + unit;
            //else
            {
                if (m_strPrefixNOT.isEmpty())
                    result = keyword + ":>" + QString::number(m_ui.minimum1->value() - 1) + unit +
                        ' ' + keyword + ":<" + QString::number(m_ui.minimum1->value() + 1) + unit;
                else
                    result = keyword + ":<" + QString::number(m_ui.minimum1->value()) + unit +
                        " OR " + keyword + ":>" + QString::number(m_ui.minimum1->value()) + unit;
            }
            break;
        case 3:
            // between...
           /* if (keyword == "length")
            {
                if (m_strPrefixNOT.isEmpty())
                    result = "length:>" + QString::number( m_ui.minimum1->value() * 60 + m_ui.minimum2->value() - 1) + unit
                        + " length:<" + QString::number( m_ui.maximum1->value() * 60 + m_ui.maximum2->value() + 1) + unit;
                else
                    result = "length:<" + QString::number( m_ui.minimum1->value() * 60 + m_ui.minimum2->value()) + unit
                        + " OR length:>" + QString::number( m_ui.maximum1->value() * 60 + m_ui.maximum2->value()) + unit;
            }*/
            //else
            {
                if (m_strPrefixNOT.isEmpty())
                    result = keyword + ":>" + QString::number(m_ui.minimum1->value() - 1) + unit +
                        ' ' + keyword + ":<" + QString::number(m_ui.maximum1->value() + 1) + unit;
                else
                    result = keyword + ":<" + QString::number(m_ui.minimum1->value() - 1) + unit +
                        " OR " + keyword + ":>" + QString::number(m_ui.maximum1->value() + 1) + unit;
            }
            break;
    }

    return result;
}

// SLOTS
void EditFilterDialog::selectedKeyword(int index) // SLOT
{
    debug() << "you selected index " << index << ": '" << m_ui.keywordCombo->currentText() << "'";
    m_ui.groupBox_2->setEnabled( false );
    m_ui.invertButton->setEnabled( true );

    const QString key = m_vector[index];
    if( index == 0 )
    {
        // Simple Search
        m_ui.groupBox_2->setEnabled( true );
        m_ui.invertButton->setEnabled( false );
        textWanted();
    }
    else if( key == "bitrate" )
    {
        // bitrate: set useful values for the spinboxes
        m_ui.minimum1->setValue( 128 );
        m_ui.maximum1->setValue( 384 );
        valueWanted();
    }
    else if( key == "samplerate" )
    {
        // samplerate: set useful values for the spinboxes
        m_ui.minimum1->setValue( 8000 );
        m_ui.maximum1->setValue( 48000 );
        valueWanted();
    }
    else if( key == "size" || key == "filesize" )
    {
        // size: set useful values for the spinboxes`
        m_ui.minimum1->setValue( 1 );
        m_ui.maximum1->setValue( 3 );
        //m_unitSizeCombo->setCurrentIndex( 2 );
        valueWanted();
    }
    else if( key == "year" )
    {
        // year: set useful values for the spinboxes
        m_ui.minimum1->setValue( 1900 );
        m_ui.maximum1->setValue( QDate::currentDate().year() );
        valueWanted();
    }
    else if( key == "track" || key == "disc" || key == "discnumber" )
    {
        // track/disc: set useful values for the spinboxes
        m_ui.minimum1->setValue( 1 );
        m_ui.maximum1->setValue( 15 );
        valueWanted();
    }
    else if( key == "score" || key == "playcount" )
    {
        m_ui.minimum1->setValue( 0 );
        m_ui.maximum1->setValue( 100 );
        valueWanted();
    }
    else if(   key == "lastplayed"
            || key == "rating"
            || key == "bpm" )
    {
        valueWanted();
    }
    //FIXME: PORT 2.0
//     else if( key=="label" )
//         textWanted( CollectionDB::instance()->labelList() );
     else if( key == "album" )
         textWanted( m_albums );
     else if( key == "artist" )
         textWanted( m_artists );
     else if( key == "composer" )
         textWanted( m_composers );
     else if( key == "genre" )
         textWanted( m_genres );
    else if( key == "type" || key == "filetype" )
    {
        QStringList types;
        types << "mp3" << "flac" << "ogg" << "oga" << "aac" << "m4a" << "m4b" << "mp4" << "mp2" << "ac3"
            << "wav" << "asf" << "wma";
        textWanted( types );
    }
    else
        textWanted();

    // assign the correct value to the m_strPrefixNOT
    assignPrefixNOT();

    // assign the right index
    m_selectedIndex = index;
}

void EditFilterDialog::minSpinChanged(int value) // SLOT
{
  if (value > m_ui.maximum1->value())
    m_ui.maximum1->setValue(value);
}

void EditFilterDialog::maxSpinChanged(int value) // SLOT
{
  if (m_ui.minimum1->value() > value)
    m_ui.minimum1->setValue(value);
}

void EditFilterDialog::textWanted() // SLOT
{
    m_ui.editKeywordBox->setEnabled( true );
    m_ui.groupBox->setEnabled( false );

    m_ui.editKeywordBox->completionObject()->clear();
}

void EditFilterDialog::textWanted( const QStringList &completion ) // SLOT
{
    m_ui.editKeywordBox->setEnabled( true );
    m_ui.groupBox->setEnabled( false );

    m_ui.editKeywordBox->completionObject()->clear();
    m_ui.editKeywordBox->completionObject()->insertItems( completion );
    m_ui.editKeywordBox->completionObject()->setIgnoreCase( true );
    m_ui.editKeywordBox->setCompletionMode( KGlobalSettings::CompletionPopup );
}

void EditFilterDialog::valueWanted() // SLOT
{
    m_ui.editKeywordBox->setEnabled( false );
    m_ui.groupBox->setEnabled( true );
}

void EditFilterDialog::chooseCondition( int condition ) // SLOT
{
    if( condition == 3 ) // included between
        chooseMinMaxValue();
    else
        chooseOneValue();
}

void EditFilterDialog::chooseOneValue() // SLOT
{
    m_ui.andLabel->setEnabled( false);
    m_ui.maximum1->setEnabled( false );
}

void EditFilterDialog::chooseMinMaxValue() // SLOT
{
    m_ui.andLabel->setEnabled( true );
    m_ui.maximum1->setEnabled( true );
}

void EditFilterDialog::slotCheckAll() // SLOT
{
    exclusiveSelectOf( 0 );
}

void EditFilterDialog::slotCheckAtLeastOne() // SLOT
{
    exclusiveSelectOf( 1 );
}

void EditFilterDialog::slotCheckExactly() // SLOT
{
    exclusiveSelectOf( 2 );
}

void EditFilterDialog::slotCheckExclude() // SLOT
{
    exclusiveSelectOf( 3 );
}

void EditFilterDialog::slotCheckAND() // SLOT
{
    m_ui.andButton->setChecked( true );
    m_ui.orButton->setChecked( false );
}

void EditFilterDialog::slotCheckOR() // SLOT
{
    m_ui.andButton->setChecked( false );
    m_ui.orButton->setChecked( true );
}

void EditFilterDialog::assignPrefixNOT() // SLOT
{
    if (m_ui.invertButton->isChecked())
        m_strPrefixNOT = '-';
    else
        m_strPrefixNOT.clear();
}

void EditFilterDialog::slotDefault() // SLOT
{
    // now append the filter rule if not empty
    if (m_ui.editKeywordBox->text().isEmpty() && (m_selectedIndex == 0))
    {
        KMessageBox::sorry( 0, i18n("<p>Sorry but the filter rule cannot be set. The text field is empty. "
                    "Please type something into it and retry.</p>"), i18n("Empty Text Field"));
        m_ui.editKeywordBox->setFocus();
        return;
    }
    if (!m_appended)
    {
        // it's the first rule
        m_appended = true;
        m_ui.groupBox_3->setEnabled( true );
    }

    m_previousFilterText = m_filterText;
    if (!m_filterText.isEmpty())
    {
        m_filterText += ' ';
        if (m_ui.orButton->isChecked())
            m_filterText += "OR ";
    }
    QStringList list = m_ui.editKeywordBox->text().split( ' ' );
    const QString key = m_vector[m_selectedIndex];
    if( m_selectedIndex == 0 )
    {
        // Simple Search
        debug() << "selected text: '" << m_ui.editKeywordBox->text() << "'";
        if (m_checkActions[0]->isChecked())
        {
            // all words
            m_filterText += m_ui.editKeywordBox->text();
        }
        else if (m_checkActions[1]->isChecked())
        {
            // at least one word
            m_filterText += *(list.constBegin());
            for ( QStringList::ConstIterator it = ++list.constBegin(), end = list.constEnd(); it != end; ++it )
                m_filterText += " OR " + *it;
        }
        else if (m_checkActions[2]->isChecked())
        {
            // exactly the words
            m_filterText += "\"" + m_ui.editKeywordBox->text() + "\"";
        }
        else if (m_checkActions[3]->isChecked())
        {
            // exclude words
            for ( QStringList::ConstIterator it = list.constBegin(), end = list.constEnd(); it != end; ++it )
                m_filterText += " -" + *it;
        }
    }
    else if( key=="bitrate"
            || key=="disc" || key=="discnumber"
            || key=="length"
            || key=="playcount"
            || key=="rating"
            || key=="samplerate"
            || key=="score"
            || key=="filesize" || key=="size"
            || key=="track"
            || key=="year" )
    {
        m_filterText += keywordConditionString( m_vector[m_selectedIndex] );
    }
    else
    {
        m_filterText += m_vector[m_selectedIndex] + ":\"" +  m_ui.editKeywordBox->text() + "\"";
    }
    emit filterChanged( m_filterText );

    m_ui.editKeywordBox->clear();
}

void EditFilterDialog::slotUser1() // SLOT
{
    m_previousFilterText = m_filterText;
    m_filterText = "";

    // no filter appended cause all cleared
    m_appended = false;
    m_ui.groupBox_3->setEnabled( false );

    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotUser2() // SLOT
{
    m_filterText = m_previousFilterText;
    if (m_filterText.isEmpty())
    {
        // no filter appended cause all cleared
        m_appended = false;
        m_ui.groupBox_3->setEnabled( false );
    }
    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotOk() // SLOT
{
    // If there's a filter typed in but unadded, add it.
    // This makes it easier to just add one condition - you only need to press OK.
    if ( !m_ui.editKeywordBox->text().isEmpty() )
        slotDefault();

    // Don't let OK do anything if they haven't set any filters.
    if (m_appended)
        accept();
}

void
EditFilterDialog::resultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    Q_UNUSED( collectionId )
    foreach( Meta::AlbumPtr album, albums )
    {
        m_albums << album->name();
    }
}

void
EditFilterDialog::resultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )
    foreach( Meta::ArtistPtr artist, artists )
    {
        m_artists << artist->name();
    }
}

void
EditFilterDialog::resultReady( const QString &collectionId, const Meta::ComposerList &composers )
{
    Q_UNUSED( collectionId )
    foreach( Meta::ComposerPtr composer, composers )
    {
        m_composers << composer->name();
    }
}

void
EditFilterDialog::resultReady( const QString &collectionId, const Meta::GenreList &genres )
{
    Q_UNUSED( collectionId )
    foreach( Meta::GenrePtr genre, genres )
    {
        m_genres << genre->name();
    }
}

#include "EditFilterDialog.moc"

