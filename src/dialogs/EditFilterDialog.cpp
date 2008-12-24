/******************************************************************************
 * Copyright (C) 2006 Giovanni Venturi <giovanni@kde-it.org>                  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "EditFilterDialog.h"

#include "amarokconfig.h"
#include "Debug.h"

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
    : KDialog( parent ),
      m_minMaxRadio(0),
      m_filterText(text),
      m_appended( false )
{
    setCaption( i18n("Edit Filter") );
    setModal( true );
    setButtons( User1|User2|Default|Ok|Cancel );
    setDefaultButton( Cancel );
    showButtonSeparator( true );


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

    setMainWidget( new QWidget( this ) );

    m_mainLayout = new QVBoxLayout( mainWidget() );

    // text explanation of this dialog
    QLabel *mainLabel = new QLabel( mainWidget() );
    mainLabel->setObjectName( "mainLabel" );
    mainLabel->setText( i18n("<p>Edit the filter for finding tracks with specific attributes"
                             ", e.g. you can look for a track that has a length of three minutes.</p>") );
    m_mainLayout->addWidget( mainLabel );
    
    m_mainLayout->addItem( new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum ) );

    // choosing keyword filtering
    QHBoxLayout *keywordLayout = new QHBoxLayout( mainWidget() );
    
    QLabel *attributeLabel = new QLabel( i18n("Attribute:"), mainWidget() );
    attributeLabel->setObjectName( "attributeLabel" );
    attributeLabel->setWhatsThis(
      i18nc("you can translate the keyword as you will do for the combobox",
           "<p>Here you can choose to <i>Simple Search</i> directly or to use "
           "some keywords to specify some attributes, such as the artist name "
           "and so on. The keywords selectable are divided by their specific value. "
           "Some keywords are numeric and others are alphanumeric. You do not need "
           "to know it directly. When a keyword is numeric it will be used to search "
           "the numeric data for each track.</p><p>The alphanumeric "
           "keywords are the following: <b>album</b>, <b>artist</b>, <b>filename</b> "
           " (including path), <b>mountpoint</b> (e.g. /home/user1), <b>filetype</b> "
           " (you can specify mp3, ogg, flac, ... and the file extensions will be matched), "
           "<b>genre</b>, <b>comment</b>, <b>composer</b>, <b>directory</b>, <b>lyrics</b>, "
           "<b>title</b>, and <b>label</b>.</p>"
           "<p>The numeric keywords are: <b>bitrate</b>, <b>disc/discnumber</b>, "
           "<b>length</b> (expressed in seconds), <b>playcount</b>, <b>rating</b>, "
           "<b>samplerate</b>, <b>score</b>, <b>size/filesize</b> (expressed in bytes, "
           "kbytes, and megabytes as specified in the unit for the filesize keyword), "
           "<b>track</b> (i.e. the track number), and <b>year</b>.</p>") );
    keywordLayout->addWidget( attributeLabel );
    
    keywordLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );
    
    m_keywordCombo = new QComboBox( mainWidget() );
    m_keywordCombo->setObjectName( "keywordComboBox" );
    m_keywordCombo->setToolTip( i18n("Select an attribute for the filter") );
    attributeLabel->setBuddy( m_keywordCombo );

    m_keywordCombo->addItem( i18n("Simple Search") );
    m_vector.push_back("Simple Search");
    m_keywordCombo->addItem( i18n("Album") );
    m_vector.push_back( "album" );
    m_keywordCombo->addItem( i18n("Artist") );
    m_vector.push_back( "artist" );
    m_keywordCombo->addItem( i18n("Composer") );
    m_vector.push_back( "composer" );
    m_keywordCombo->addItem( i18n("Genre") );
    m_vector.push_back( "genre" );
    m_keywordCombo->addItem( i18n("Title") );
    m_vector.push_back( "title" );
    m_keywordCombo->addItem( i18n("Track") );
    m_vector.push_back( "track" );
    m_keywordCombo->addItem( i18n("Year") );
    m_vector.push_back( "year" );

    // the "Simple Search" text is selected in the comboKeyword
    m_selectedIndex = 0;

    keywordLayout->addWidget( m_keywordCombo );
    keywordLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );
    
    m_keywordEdit = new KLineEdit( mainWidget() );
    m_keywordEdit->setObjectName( "editKeywordBox" );
    m_keywordEdit->setWhatsThis( i18n("<p>Type the attribute value or the text to look for here.</p>") );
    keywordLayout->addWidget( m_keywordEdit );
    
    m_mainLayout->addLayout( keywordLayout );
    m_mainLayout->addItem( new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
    
    connect(m_keywordCombo, SIGNAL(activated(int)), this, SLOT(selectedKeyword(int)));

    // group of options on numeric attribute keywords: a value <,>,= ... or a value between Min and Max
    m_groupBox = new QGroupBox( mainWidget() );
    m_groupBox->setTitle( i18n( "Attribute value is" ) );
    m_mainLayout->addWidget( m_groupBox );
    m_mainLayout->addItem( new QSpacerItem( 10, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    QVBoxLayout *vertLayout = new QVBoxLayout( m_groupBox );
    vertLayout->setMargin( 15 );
    vertLayout->setSpacing( 5 );

    // choose other keyword parameters: smaller than, greater than, equal to...
    QHBoxLayout *paramLayout = new QHBoxLayout();
    vertLayout->addLayout( paramLayout );

    m_conditionCombo = new QComboBox( m_groupBox );
    m_conditionCombo->setObjectName( "valuecondition" );
    m_conditionCombo->addItem( i18n("smaller than") );
    m_conditionCombo->addItem( i18n("larger than") );
    m_conditionCombo->addItem( i18n("equal to") );
    m_conditionCombo->addItem( i18n("between") );
    paramLayout->addWidget( m_conditionCombo );
    paramLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Fixed, QSizePolicy::Minimum ) );

    m_minimumSpin1 = new QSpinBox( m_groupBox );
    m_minimumSpin1->setObjectName( "minimum1" );
    paramLayout->addWidget( m_minimumSpin1 );
    
    paramLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    m_minimumSpin2 = new QSpinBox( m_groupBox );
    m_minimumSpin2->setObjectName( "minimum2" );
    paramLayout->addWidget( m_minimumSpin2 );
    paramLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    connect(m_minimumSpin1, SIGNAL(valueChanged(int)), this, SLOT(minSpinChanged(int)));

    m_andLabel = new QLabel( i18n("and"), m_groupBox );
    m_andLabel->setObjectName( "andLabel" );
    paramLayout->addWidget( m_andLabel );
    paramLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    m_maxSpin1 = new QSpinBox( m_groupBox );
    m_maxSpin1->setObjectName( "maximum1" );
    paramLayout->addWidget( m_maxSpin1 );
    
    paramLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    m_maxSpin2 = new QSpinBox( m_groupBox );
    m_maxSpin2->setObjectName( "maximum2" );
    paramLayout->addWidget( m_maxSpin2 );

    connect(m_maxSpin1, SIGNAL(valueChanged(int)), this, SLOT(maxSpinChanged(int)));

    QHBoxLayout *filesizeLayout = new QHBoxLayout();
    vertLayout->addLayout( filesizeLayout );
    filesizeLayout->setAlignment( Qt::AlignLeft );
    
    m_filesizeLabel = new QLabel( i18n( "Unit:" ), m_groupBox );
    m_filesizeLabel->setObjectName( "filesizeLabel" );
    filesizeLayout->addWidget( m_filesizeLabel );
    
    filesizeLayout->addItem( new QSpacerItem( 5, 10, QSizePolicy::Fixed, QSizePolicy::Minimum ) );
    
    m_unitSizeCombo = new QComboBox( m_groupBox );
    m_unitSizeCombo->setObjectName( "comboUnitSize" );
    m_filesizeLabel->setBuddy( m_unitSizeCombo );
    m_unitSizeCombo->addItem( i18n("B (1 Byte)") );
    m_unitSizeCombo->addItem( i18n("KB (1024 Bytes)") );
    m_unitSizeCombo->addItem( i18n("MB (1024 KB)") );
    filesizeLayout->addWidget( m_unitSizeCombo );

    // type text selected
    textWanted();

    // check the "One Value Choosing" by default
    chooseOneValue();

    connect( m_conditionCombo, SIGNAL(activated(int)), SLOT(chooseCondition(int)) );

    QHBoxLayout *otherOptionsLayout = new QHBoxLayout( mainWidget() );
    otherOptionsLayout->setAlignment( Qt::AlignHCenter );
    m_mainLayout->addLayout( otherOptionsLayout );

    // the groupbox to select the action filter
    m_groupBox2 = new QGroupBox( mainWidget() );
    m_groupBox2->setTitle( i18n( "Filter action" ) );
    otherOptionsLayout->addWidget( m_groupBox2 );

    QVBoxLayout* ratioLayout = new QVBoxLayout( m_groupBox2 );
    ratioLayout->setMargin( 15 );
    ratioLayout->setSpacing( 0 );

    m_matchAllButton = new QRadioButton( i18n("Match all words"), m_groupBox2 );
    m_matchAllButton->setObjectName( "checkall" );
    m_matchAllButton->setToolTip(
      i18n("<p>Check this box to look for the tracks that contain all the words you typed "
           "in the related Simple Search edit box</p>"));
    ratioLayout->addWidget( m_matchAllButton );

    m_matchOneButton = new QRadioButton( i18n("Match any word"), m_groupBox2 );
    m_matchOneButton->setObjectName( "checkor" );
    m_matchOneButton->setToolTip(
      i18n("<p>Check this box to look for the tracks that contain at least one of the words "
           "you typed in the related Simple Search edit box</p>"));
    ratioLayout->addWidget( m_matchOneButton );

    m_matchExactlyButton = new QRadioButton( i18n("Exact match"), m_groupBox2 );
    m_matchExactlyButton->setObjectName( "checkexactly" );
    m_matchExactlyButton->setToolTip(
      i18n("<p>Check this box to look for all the tracks that contain exactly the words you typed "
           "in the related Simple Search edit box</p>"));
    ratioLayout->addWidget( m_matchExactlyButton );

    m_excludeMatchButton = new QRadioButton( i18n("Exclude"), m_groupBox2 );
    m_excludeMatchButton->setObjectName( "checkexclude" );
    m_excludeMatchButton->setToolTip(
      i18n("<p>Check this box to look for all the tracks that do not contain the words you typed "
           "in the related Simple Search edit box</p>"));
    ratioLayout->addWidget( m_excludeMatchButton );

    m_checkActions << m_matchAllButton;
    m_checkActions << m_matchOneButton;
    m_checkActions << m_matchExactlyButton;
    m_checkActions << m_excludeMatchButton;

    connect( m_matchAllButton, SIGNAL(clicked()), this, SLOT(slotCheckAll()) );
    connect( m_matchOneButton, SIGNAL(clicked()), this, SLOT(slotCheckAtLeastOne()) );
    connect( m_matchExactlyButton, SIGNAL(clicked()), this, SLOT(slotCheckExactly()) );
    connect( m_excludeMatchButton, SIGNAL(clicked()), this, SLOT(slotCheckExclude()) );

    // check "select all words" as default
    slotCheckAll();

    // some vertical space
    otherOptionsLayout->addItem( new QSpacerItem( 50, 5, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    QVBoxLayout* verticalConditionLayout = new QVBoxLayout();
    otherOptionsLayout->addLayout( verticalConditionLayout );
    verticalConditionLayout->setMargin( 15 );
    verticalConditionLayout->setSpacing( 0 );

    m_groupBox3 = new QGroupBox( mainWidget() );
    m_groupBox3->setTitle( i18n( "Appending condition" ) );
    verticalConditionLayout->addWidget( m_groupBox3 );

    QVBoxLayout* ratioLayout2 = new QVBoxLayout( m_groupBox3 );
    ratioLayout2->setMargin(15);
    ratioLayout2->setSpacing(0);

    m_andButton = new QRadioButton( i18nc("AND logic condition", "AND"), m_groupBox3 );
    m_andButton->setObjectName( "checkAND" );
    m_andButton->setToolTip(
      i18n("<p>Check this box if you want to add another condition and you want that the filter "
           "to match both the previous conditions and this new one</p>"));
    ratioLayout2->addWidget( m_andButton );

    m_orButton = new QRadioButton( i18nc("OR logic condition", "OR"), m_groupBox3 );
    m_orButton->setObjectName( "checkOR" );
    m_orButton->setToolTip(
      i18n("<p>Check this box if you want to add another condition and you want that the filter "
           "to match either the previous conditions or this new one</p>"));
    ratioLayout2->addWidget( m_orButton );

    otherOptionsLayout->addItem( new QSpacerItem( 10, 10, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    m_invertButton = new QCheckBox( i18n("Invert condition"), mainWidget() );
    m_invertButton->setObjectName( "prefixNOT" );
    m_invertButton->setToolTip(
      i18n("Check this box to negate the defined filter condition"));
    m_invertButton->setWhatsThis(
      i18n("<p>If this option is checked the defined filter condition will be negated. "
           "This means that, for example, you can define a filter that looks for all "
           "tracks that are not of a specific album, artist, and so on.</p>"));
    verticalConditionLayout->addWidget( m_invertButton );
    m_invertButton->setEnabled( false );

    connect(m_invertButton, SIGNAL(clicked()), SLOT(assignPrefixNOT()));

    m_mainLayout->addItem( new QSpacerItem( 10, 20, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

    // you need to append at least one filter condition to specify if do
    // an "AND" or an "OR" with the next condition if the filter is empty
    if( m_filterText.isEmpty() )
      m_groupBox3->setEnabled( false );

    connect( m_andButton, SIGNAL(clicked()), SLOT(slotCheckAND()) );
    connect( m_orButton, SIGNAL(clicked()), SLOT(slotCheckOR()) );

    // check "AND" condition as default
    slotCheckAND();

    // setup Min Max Value spin
    setMinMaxValueSpins();
    connect( this, SIGNAL(okClicked()), this, SLOT(slotOk() ) );
    connect( this, SIGNAL( defaultClicked() ) , this, SLOT(slotDefault() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
    connect( this, SIGNAL( user2Clicked() ), this, SLOT( slotUser2() ) );
}

EditFilterDialog::~EditFilterDialog()
{
    delete m_keywordEdit;
}

QString EditFilterDialog::filter() const
{
    return m_filterText;
}

void EditFilterDialog::exclusiveSelectOf( int which )
{
    int size = static_cast<int>( m_checkActions.count() );

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

    if (m_vector.at(m_selectedIndex) == "size")
    {
        switch (m_unitSizeCombo->currentIndex())
        {
            case 1:
                // kbytes
                unit = "k";
                break;
            case 2:
                // mbytes
                unit = "m";
                break;
        }
    }

    switch(m_conditionCombo->currentIndex())
    {
        case 0:
            // less than...
            result = m_strPrefixNOT + keyword + ":<";
            if (keyword == "length")
                result += QString::number( m_minimumSpin1->value() * 60 + m_minimumSpin2->value() ) + unit;
            else
                result += m_minimumSpin1->text() + unit;
            break;
        case 1:
            // greater than...
            result = m_strPrefixNOT + keyword + ":>";
            if (keyword == "length")
                result += QString::number( m_minimumSpin1->value() * 60 + m_minimumSpin2->value() ) + unit;
            else
                result += m_minimumSpin1->text() + unit;
            break;
        case 2:
            // equal to...
            if (keyword == "length")
                result = m_strPrefixNOT + "length:" + QString::number( m_minimumSpin1->value() * 60
                        + m_minimumSpin2->value() ) + unit;
            else
            {
                if (m_strPrefixNOT.isEmpty())
                    result = keyword + ":>" + QString::number(m_minimumSpin1->value() - 1) + unit +
                        ' ' + keyword + ":<" + QString::number(m_minimumSpin1->value() + 1) + unit;
                else
                    result = keyword + ":<" + QString::number(m_minimumSpin1->value()) + unit +
                        " OR " + keyword + ":>" + QString::number(m_minimumSpin1->value()) + unit;
            }
            break;
        case 3:
            // between...
            if (keyword == "length")
            {
                if (m_strPrefixNOT.isEmpty())
                    result = "length:>" + QString::number( m_minimumSpin1->value() * 60 + m_minimumSpin2->value() - 1) + unit
                        + " length:<" + QString::number( m_maxSpin1->value() * 60 + m_maxSpin2->value() + 1) + unit;
                else
                    result = "length:<" + QString::number( m_minimumSpin1->value() * 60 + m_minimumSpin2->value()) + unit
                        + " OR length:>" + QString::number( m_maxSpin1->value() * 60 + m_maxSpin2->value()) + unit;
            }
            else
            {
                if (m_strPrefixNOT.isEmpty())
                    result = keyword + ":>" + QString::number(m_minimumSpin1->value() - 1) + unit +
                        ' ' + keyword + ":<" + QString::number(m_maxSpin1->value() + 1) + unit;
                else
                    result = keyword + ":<" + QString::number(m_minimumSpin1->value() - 1) + unit +
                        " OR " + keyword + ":>" + QString::number(m_maxSpin1->value() + 1) + unit;
            }
            break;
    }

    return result;
}

void EditFilterDialog::setMinMaxValueSpins()
{
    // setting some spin box options and limit values
    m_minimumSpin1->setValue( 0 );
    m_minimumSpin1->setMinimum( 0 );
    m_minimumSpin1->setMaximum( 100000000 );

    m_minimumSpin2->setMinimum( 0 );
    m_minimumSpin2->setMaximum( 59 );
    m_minimumSpin2->hide();

    m_maxSpin1->setValue( 0 );
    m_maxSpin1->setMinimum( 0 );
    m_maxSpin1->setMaximum( 100000000 );

    m_maxSpin2->setMinimum( 0 );
    m_maxSpin2->setMaximum( 59 );
    m_maxSpin2->hide();

    // fix tooltip
    m_minimumSpin1->setToolTip( "" );
    m_minimumSpin2->setToolTip( i18n("Seconds") );

    m_maxSpin1->setToolTip( "" );
    m_maxSpin2->setToolTip( i18n("Seconds") );
}

// SLOTS
void EditFilterDialog::selectedKeyword(int index) // SLOT
{
    debug() << "you selected index " << index << ": '" << m_keywordCombo->currentText() << "'";
    m_groupBox2->setEnabled( false );
    m_unitSizeCombo->setEnabled( false );
    m_filesizeLabel->setEnabled( false );
    m_invertButton->setEnabled( true );

    setMinMaxValueSpins();

    const QString key = m_vector[index];
    if( index == 0 )
    {
        // Simple Search
        m_groupBox2->setEnabled( true );
        m_invertButton->setEnabled( false );
        textWanted();
    }
    else if( key=="bitrate" )
    {
        // bitrate: set useful values for the spinboxes
        m_minimumSpin1->setValue( 128 );
        m_maxSpin1->setValue( 384 );
        valueWanted();
    }
    else if( key=="samplerate" )
    {
        // samplerate: set useful values for the spinboxes
        m_minimumSpin1->setValue( 8000 );
        m_maxSpin1->setValue( 48000 );
        valueWanted();
    }
    else if( key=="length" )
    {
        // length: set useful values for the spinboxes
        m_minimumSpin2->show();
        m_maxSpin2->show();
        m_minimumSpin1->setValue( 1 );
        m_maxSpin1->setValue( 5 );
        m_minimumSpin1->setToolTip( i18n("Minutes") );
        m_maxSpin1->setToolTip( i18n("Minutes") );

        // fix the maximum values to reduce spinboxes size
        m_minimumSpin1->setMaximum( 240 );
        m_maxSpin1->setMaximum( 240 );

        valueWanted();
    }
    else if( key=="size" || key=="filesize" )
    {
        // size: set useful values for the spinboxes
        m_filesizeLabel->setEnabled( true );
        m_unitSizeCombo->setEnabled( true );
        m_minimumSpin1->setValue( 1 );
        m_maxSpin1->setValue( 3 );
        m_unitSizeCombo->setCurrentIndex( 2 );
        valueWanted();
    }
    else if( key=="year" )
    {
        // year: set useful values for the spinboxes
        m_minimumSpin1->setValue( 1900 );
        m_maxSpin1->setValue( QDate::currentDate().year() );
        valueWanted();
    }
    else if( key=="track" || key=="disc" || key=="discnumber" )
    {
        // track/disc: set useful values for the spinboxes
        m_minimumSpin1->setValue( 1 );
        m_maxSpin1->setValue( 15 );
        valueWanted();
    }
    else if( key=="playcount"
            || key=="lastplayed"
            || key=="rating"
            || key=="score"
            || key=="bpm" )
    {
        valueWanted();
    }
    //FIXME: PORT 2.0
//     else if( key=="label" )
//         textWanted( CollectionDB::instance()->labelList() );
//     else if( key=="album" )
//         textWanted( CollectionDB::instance()->albumList() );
//     else if( key=="artist" )
//         textWanted( CollectionDB::instance()->artistList() );
//     else if( key=="composer" )
//         textWanted( CollectionDB::instance()->composerList() );
//     else if( key=="genre" )
//         textWanted( CollectionDB::instance()->genreList() );
    else if( key=="type" || key=="filetype" )
    {
        QStringList types;
        types << "mp3" << "flac" << "ogg" << "oga" << "aac" << "m4a" << "mp4" << "mp2" << "ac3"
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
  if (value > m_maxSpin1->value())
    m_maxSpin1->setValue(value);
}

void EditFilterDialog::maxSpinChanged(int value) // SLOT
{
  if (m_minimumSpin1->value() > value)
    m_minimumSpin1->setValue(value);
}

void EditFilterDialog::textWanted() // SLOT
{
    m_keywordEdit->setEnabled( true );
    m_groupBox->setEnabled( false );

    m_keywordEdit->completionObject()->clear();
}

void EditFilterDialog::textWanted( const QStringList &completion ) // SLOT
{
    m_keywordEdit->setEnabled( true );
    m_groupBox->setEnabled( false );

    m_keywordEdit->completionObject()->clear();
    m_keywordEdit->completionObject()->insertItems( completion );
    m_keywordEdit->completionObject()->setIgnoreCase( true );
    m_keywordEdit->setCompletionMode( KGlobalSettings::CompletionPopup );
}

void EditFilterDialog::valueWanted() // SLOT
{
    m_keywordEdit->setEnabled( false );
    m_groupBox->setEnabled( true );
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
    m_andLabel->setEnabled( false);
    m_maxSpin1->setEnabled( false );
    m_maxSpin2->setEnabled( false );
}

void EditFilterDialog::chooseMinMaxValue() // SLOT
{
    m_andLabel->setEnabled( true );
    m_maxSpin1->setEnabled( true );
    m_maxSpin2->setEnabled( true );
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
    m_andButton->setChecked( true );
    m_orButton->setChecked( false );
}

void EditFilterDialog::slotCheckOR() // SLOT
{
    m_andButton->setChecked( false );
    m_orButton->setChecked( true );
}

void EditFilterDialog::assignPrefixNOT() // SLOT
{
    if (m_invertButton->isChecked())
        m_strPrefixNOT = "-";
    else
        m_strPrefixNOT = "";
}

void EditFilterDialog::slotDefault() // SLOT
{
    // now append the filter rule if not empty
    if (m_keywordEdit->text().isEmpty() && (m_selectedIndex == 0))
    {
        KMessageBox::sorry( 0, i18n("<p>Sorry but the filter rule cannot be set. The text field is empty. "
                    "Please type something into it and retry.</p>"), i18n("Empty Text Field"));
        m_keywordEdit->setFocus();
        return;
    }
    if (!m_appended)
    {
        // it's the first rule
        m_appended = true;
        m_groupBox3->setEnabled( true );
    }

    m_previousFilterText = m_filterText;
    if (!m_filterText.isEmpty())
    {
        m_filterText += ' ';
        if (m_orButton->isChecked())
            m_filterText += "OR ";
    }
    QStringList list = m_keywordEdit->text().split( ' ' );
    const QString key = m_vector[m_selectedIndex];
    if( m_selectedIndex == 0 )
    {
        // Simple Search
        debug() << "selected text: '" << m_keywordEdit->text() << "'";
        if (m_checkActions[0]->isChecked())
        {
            // all words
            m_filterText += m_keywordEdit->text();
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
            m_filterText += "\"" + m_keywordEdit->text() + "\"";
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
        m_filterText += m_vector[m_selectedIndex] + ":\"" +  m_keywordEdit->text() + "\"";
    }
    emit filterChanged( m_filterText );

    m_keywordEdit->clear();
}

void EditFilterDialog::slotUser1() // SLOT
{
    m_previousFilterText = m_filterText;
    m_filterText = "";

    // no filter appended cause all cleared
    m_appended = false;
    m_groupBox3->setEnabled( false );

    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotUser2() // SLOT
{
    m_filterText = m_previousFilterText;
    if (m_filterText.isEmpty())
    {
        // no filter appended cause all cleared
        m_appended = false;
        m_groupBox3->setEnabled( false );
    }
    emit filterChanged( m_filterText );
}

void EditFilterDialog::slotOk() // SLOT
{
    // If there's a filter typed in but unadded, add it.
    // This makes it easier to just add one condition - you only need to press OK.
    if ( !m_keywordEdit->text().isEmpty() )
        slotDefault();

    // Don't let OK do anything if they haven't set any filters.
    if (m_appended)
        accept();
}

#include "EditFilterDialog.moc"

