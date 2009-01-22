/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
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
#include "FilenameLayoutDialog.h"

#include "Amarok.h"
#include "Debug.h"

#include <KConfig>
#include <KColorScheme>


FilenameLayoutDialog::FilenameLayoutDialog( QWidget *parent, bool isOrganizeCollection )
    : QWidget( parent )
    , m_isOrganizeCollection( isOrganizeCollection )
    , m_advancedMode( false )
{
    setupUi( this );
    optionsFrame->hide();

    m_caseEditRadioButtons << rbAllUpper << rbAllLower << rbFirstLetter << rbTitleCase;

    filenameLayoutEdit->hide();
    syntaxLabel->hide();
    syntaxLabel->setWordWrap( true );

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( editStateEnable( bool ) ) );
    connect( parent, SIGNAL( accepted() ),
             this, SLOT( onAccept() ) );
    connect( tokenPool, SIGNAL( onDoubleClick( QString ) ),
             filenameLayout, SLOT( addToken( QString ) ) );
    connect( kpbAdvanced, SIGNAL( clicked() ),
             this, SLOT( toggleAdvancedMode() ) );
    connect( filenameLayout, SIGNAL( schemeChanged() ),
             this, SIGNAL( schemeChanged() ) );
    connect( filenameLayoutEdit, SIGNAL( textChanged( const QString & ) ),
             this, SIGNAL( schemeChanged() ) );

    //KConfig stuff:
    int caseOptions = Amarok::config( "TagGuesser" ).readEntry( "Case options" ).toInt();
    if( !caseOptions )
        cbCase->setChecked( false );
    else
    {
        cbCase->setChecked( true );
        if( caseOptions == 1 )
            rbAllLower->setChecked( true );
        else if( caseOptions == 2 )
            rbAllUpper->setChecked( true );
        else if( caseOptions == 3 )
            rbFirstLetter->setChecked( true );
        else if( caseOptions == 4 )
            rbTitleCase->setChecked( true );
        else
            debug() << "OUCH";
    }
    int whitespaceOptions = Amarok::config( "TagGuesser" ).readEntry( "Eliminate trailing spaces" ).toInt();
    cbEliminateSpaces->setChecked( whitespaceOptions );
    int underscoreOptions = Amarok::config( "TagGuesser" ).readEntry( "Replace underscores" ).toInt();
    cbReplaceUnderscores->setChecked( underscoreOptions );
    if( !m_isOrganizeCollection )
        optionsFrame->show();
    else
    {
        //INIT for collection root
        unsigned int borderColor = static_cast<unsigned int>( KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color().rgb() );
        collectionRootFrame->setStyleSheet( "\
            color: palette( Base );\
            border: 2px solid #" + QString::number( borderColor, 16 ).remove( 0, 2 ) + ";\
            border-radius: 4px;\
            padding: 2px;\
            " );
        QHBoxLayout *collectionRootLayout = new QHBoxLayout( collectionRootFrame );
        QLabel *collectionRootIconLabel = new QLabel( "", this );
        QLabel *collectionRootLabel = new QLabel( i18n( "Collection root" ), this );
        collectionRootLayout->addWidget( collectionRootIconLabel );
        collectionRootLayout->addWidget( collectionRootLabel );
        collectionRootLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
        collectionRootIconLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
        collectionRootLayout->setContentsMargins( 0, 0, 0, 0 );
        collectionRootIconLabel->setContentsMargins( 0, 0, 0, 0 );
        collectionRootLabel->setContentsMargins( 0, 0, 0, 0 );
        collectionRootIconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        collectionRootIconLabel->setFixedSize( 16, 16 );
        QPixmap collectionIcon = QPixmap( KIcon( "collection-amarok" ).pixmap(16, 16) );
        collectionRootIconLabel->setPixmap( collectionIcon );

        collectionSlashFrame->setStyleSheet( "\
            color: palette( Base );\
            border: 2px solid #" + QString::number( borderColor, 16 ).remove( 0, 2 ) + ";\
            border-radius: 4px;\
            padding: 2px;\
            " );
        QHBoxLayout *collectionSlashLayout = new QHBoxLayout( collectionSlashFrame );
        QLabel *collectionSlashLabel = new QLabel( "/", this );
        collectionSlashLayout->addWidget(collectionSlashLabel);
        collectionSlashLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
        collectionSlashLayout->setContentsMargins( 0, 0, 0, 0 );
        collectionSlashLabel->setContentsMargins( 0, 0, 0, 0 );
    }
    //INIT for tokenPool
    tokenPool->addToken( Token::Track );
    tokenPool->addToken( Token::Title );
    tokenPool->addToken( Token::Artist );
    tokenPool->addToken( Token::Composer );
    tokenPool->addToken( Token::Year );
    tokenPool->addToken( Token::Album );
    tokenPool->addToken( Token::Comment );
    tokenPool->addToken( Token::Genre );
    tokenPool->addToken( Token::Underscore );
    tokenPool->addToken( Token::Dash );
    tokenPool->addToken( Token::Dot );
    tokenPool->addToken( Token::Space );

    if( !m_isOrganizeCollection )
    {
        tokenPool->addToken( Token::Ignore );
        syntaxLabel->setText( i18nc("Please do not translate the %foo words as they define a syntax used internally by a parser to describe a filename.",
                                    // xgettext: no-c-format
                                    "The following tokens can be used to define a filename scheme: \
                                     <br>%track, %title, %artist, %composer, %year, %album, %comment, %genre, %ignore." ) );
    }
    else
    {
        tokenPool->addToken( Token::Slash );
        tokenPool->addToken( Token::Initial );
        tokenPool->addToken( Token::FileType );
        tokenPool->addToken( Token::DiscNumber );
        syntaxLabel->setText( i18nc("Please do not translate the %foo words as they define a syntax used internally by a parser to describe a filename.",
                                    // xgettext: no-c-format
                                    "The following tokens can be used to define a filename scheme: \
                                     <br>%track, %title, %artist, %composer, %year, %album, %comment, %genre, %initial, %folder, %filetype, %discnumber." ) );
    }
    if( m_isOrganizeCollection )
    {
        if( Amarok::config( "OrganizeCollectionDialog" ).readEntry( "Mode" ) == "Advanced" )
        {
            setAdvancedMode( true );
            filenameLayoutEdit->setText( Amarok::config( "OrganizeCollectionDialog" ).readEntry( "Scheme" ) );
        }
        else if( Amarok::config( "OrganizeCollectionDialog" ).readEntry( "Mode" ) == "Basic" )
        {
            setAdvancedMode( false );
            filenameLayout->inferScheme( Amarok::config( "OrganizeCollectionDialog" ).readEntry( "Scheme" ) );
        }
    }
    else
    {
        if( Amarok::config( "FilenameLayoutDialog" ).readEntry( "Mode" ) == "Advanced" )
        {
            setAdvancedMode( true );
            filenameLayoutEdit->setText( Amarok::config( "FilenameLayoutDialog" ).readEntry( "Scheme" ) );
        }
        else if( Amarok::config( "FilenameLayoutDialog" ).readEntry( "Mode" ) == "Basic" )
        {
            setAdvancedMode( false );
            filenameLayout->inferScheme( Amarok::config( "FilenameLayoutDialog" ).readEntry( "Scheme" ) );
        }
    }
}

//Stores the configuration when the dialog is accepted.
void
FilenameLayoutDialog::onAccept()    //SLOT
{
    Amarok::config( "TagGuesser" ).writeEntry( "Case options", getCaseOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Eliminate trailing spaces", getWhitespaceOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Replace underscores", getUnderscoreOptions() );
}

//Forwards the request for a scheme to FilenameLayoutWidget
QString
FilenameLayoutDialog::getParsableScheme()
{
    QString category = m_isOrganizeCollection ? "OrganizeCollectionDialog" : "FilenameLayoutDialog";
    QString scheme   = m_advancedMode ? filenameLayoutEdit->text() : filenameLayout->getParsableScheme();

    Amarok::config( category ).writeEntry( "Scheme", scheme );
    return scheme;
}

//Handles the radiobuttons
void
FilenameLayoutDialog::editStateEnable( bool checked )      //SLOT
{
    foreach( QRadioButton *rb, m_caseEditRadioButtons )
        rb->setEnabled( checked );
}

//Returns a code for the configuration.
int
FilenameLayoutDialog::getCaseOptions()
{
    //Amarok::config( "TagGuesser" ).readEntry( "Filename schemes", QStringList() );
    if( !cbCase->isChecked() )
        return 0;
    else
    {
        if( rbAllLower->isChecked() )
            return 1;
        else if( rbAllUpper->isChecked() )
            return 2;
        else if( rbFirstLetter->isChecked() )
            return 3;
        else if( rbTitleCase->isChecked() )
            return 4;
        else
        {
            debug() << "OUCH!";
            return 99;
        }
    }
}

//As above
int
FilenameLayoutDialog::getWhitespaceOptions()
{
    if( !cbEliminateSpaces->isChecked() )
        return 0;
    return 1;
}

//As above
int
FilenameLayoutDialog::getUnderscoreOptions()
{
    if( !cbReplaceUnderscores->isChecked() )
        return 0;
    return 1;
}

//Handles the modifications to the dialog to toggle between advanced and basic editing mode.
void
FilenameLayoutDialog::toggleAdvancedMode()
{
    setAdvancedMode( !m_advancedMode );
}

//handles switching between basic and advanced mode
void
FilenameLayoutDialog::setAdvancedMode( bool isAdvanced )
{
    m_advancedMode = isAdvanced;

    if( isAdvanced )
    {
        kpbAdvanced->setText( i18n( "&Basic..." ) );
        filenameLayout->hide();
        filenameLayoutEdit->show();
        filenameLayoutEdit->setText( filenameLayout->getParsableScheme() );
        tokenPool->hide();
        syntaxLabel->show();

    }
    else // set Basic mode
    {
        kpbAdvanced->setText( i18n( "&Advanced..." ) );
        filenameLayout->show();
        filenameLayout->inferScheme( filenameLayoutEdit->text() );
        filenameLayoutEdit->hide();
        tokenPool->show();
        syntaxLabel->hide();
    }

    QString configValue = m_isOrganizeCollection ? "OrganizeCollectionDialog" : "FilenameLayoutDialog";
    QString entryValue  = m_advancedMode ? "Advanced" : "Basic";

    Amarok::config( configValue ).writeEntry( "Mode", entryValue );
}

