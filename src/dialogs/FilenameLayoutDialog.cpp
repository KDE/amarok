/****************************************************************************************
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "FilenameLayoutDialog.h"
#include "TokenDropTarget.h"

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

    m_dropTarget = new TokenDropTarget( "application/x-amarok-tag-token", filenameLayout );
    m_dropTarget->setRowLimit( 1 );
    m_dropTarget->layout()->setContentsMargins( 1, 1, 1, 1 );
    QVBoxLayout *l = new QVBoxLayout(filenameLayout);
    l->setContentsMargins( 0, 0, 0, 0 );
    l->addWidget(m_dropTarget);
    
    filenameLayoutEdit->hide();
    syntaxLabel->hide();
    syntaxLabel->setWordWrap( true );

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( editStateEnable( bool ) ) );
    connect( tokenPool, SIGNAL( onDoubleClick( Token * ) ),
             m_dropTarget, SLOT( insertToken( Token* ) ) );
    connect( kpbAdvanced, SIGNAL( clicked() ),
             this, SLOT( toggleAdvancedMode() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
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
    tokenPool->addToken( new Token( i18n( "Track" ), "filename-track-amarok", Track ) );
    tokenPool->addToken( new Token( i18n( "Title" ), "filename-title-amarok", Title ) );
    tokenPool->addToken( new Token( i18n( "Artist" ), "filename-artist-amarok", Artist )) ;
    tokenPool->addToken( new Token( i18n( "Composer" ), "filename-composer-amarok", Composer ) );
    tokenPool->addToken( new Token( i18n( "Year" ), "filename-year-amarok", Year ) );
    tokenPool->addToken( new Token( i18n( "Album" ), "filename-album-amarok", Album ) );
    tokenPool->addToken( new Token( i18n( "Comment" ), "filename-comment-amarok", Comment ) );
    tokenPool->addToken( new Token( i18n( "Genre" ), "filename-genre-amarok", Genre ) );
    tokenPool->addToken( new Token( "_", "filename-underscore-amarok", Underscore ) );
    tokenPool->addToken( new Token( "-", "filename-dash-amarok", Dash ) );
    tokenPool->addToken( new Token( ".", "filename-dot-amarok", Dot ) );
    tokenPool->addToken( new Token( " ", "filename-space-amarok", Space ) );

    if( !m_isOrganizeCollection )
    {
        tokenPool->addToken( new Token( i18n( "Ignore" ), "filename-ignore-amarok", Ignore ) );
        syntaxLabel->setText( i18nc("Please do not translate the %foo words as they define a syntax used internally by a parser to describe a filename.",
                                    // xgettext: no-c-format
                                    "The following tokens can be used to define a filename scheme: \
                                     <br>%track, %title, %artist, %composer, %year, %album, %comment, %genre, %ignore." ) );
    }
    else
    {
        tokenPool->addToken( new Token( "/", "filename-slash-amarok", Slash ) );
        tokenPool->addToken( new Token( i18n( "Initial" ), "filename-initial-amarok", Initial ) );
        tokenPool->addToken( new Token( i18n( "File type" ), "filename-filetype-amarok", FileType ) );
        tokenPool->addToken( new Token( i18n( "Disc number" ), "filename-discnumber-amarok", DiscNumber ) );
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
            inferScheme( Amarok::config( "OrganizeCollectionDialog" ).readEntry( "Scheme" ) );
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
            inferScheme( Amarok::config( "FilenameLayoutDialog" ).readEntry( "Scheme" ) );
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

//Forwards the request for a scheme to TokenLayoutWidget
QString
FilenameLayoutDialog::getParsableScheme()
{
    QString category = m_isOrganizeCollection ? "OrganizeCollectionDialog" : "FilenameLayoutDialog";
    QString scheme   = m_advancedMode ? filenameLayoutEdit->text() : parsableScheme();

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
        filenameLayoutEdit->setText( parsableScheme() );
        tokenPool->hide();
        syntaxLabel->show();

    }
    else // set Basic mode
    {
        kpbAdvanced->setText( i18n( "&Advanced..." ) );
        filenameLayout->show();
        filenameLayoutEdit->hide();
        tokenPool->show();
        syntaxLabel->hide();
        inferScheme( filenameLayoutEdit->text() );
    }

    QString configValue = m_isOrganizeCollection ? "OrganizeCollectionDialog" : "FilenameLayoutDialog";
    QString entryValue  = m_advancedMode ? "Advanced" : "Basic";

    Amarok::config( configValue ).writeEntry( "Mode", entryValue );
}



// Iterates over the elements of the TokenLayoutWidget bar 
// (really over the elements of a QList that stores the indexes 
// of the tokens) and generates a string that TagGuesser can digest.
QString
FilenameLayoutDialog::parsableScheme() const
{
    QString parsableScheme = "";

    QList< Token *> list = m_dropTarget->drags( 0 );
    
    foreach( Token * token, list )
    {
        parsableScheme += typeElements[token->value()];
    }

    return parsableScheme;
}



//tries to populate the widget with tokens according to a string
void
FilenameLayoutDialog::inferScheme( const QString &s ) //SLOT
{
    DEBUG_BLOCK

    debug() << "infering scheme: " << s;

    m_dropTarget->clear();
    for( int i = 0; i < s.size(); )
    {
        if( s.at(i) == '%')
        {
            if( s.mid( i, 6 ) == "%title" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Title" ), "filename-title-amarok", Title ) );
                i += 6;
            }
            else if( s.mid( i, 6 ) == "%track" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Track" ), "filename-track-amarok", Track ) );
                i += 6;
            }
            else if( s.mid( i, 7 ) == "%artist" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Artist" ), "filename-artist-amarok", Artist ) );
                i += 7;
            }
            else if( s.mid( i, 9 ) == "%composer" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Composer" ), "filename-composer-amarok", Composer ) );
                i += 9;
            }
            else if( s.mid( i, 5 ) == "%year" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Year" ), "filename-year-amarok", Year ) );
                i += 5;
            }
            else if( s.mid( i, 6 ) == "%album" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Album" ), "filename-album-amarok", Album ) );
                i += 6;
            }
            else if( s.mid( i, 8 ) == "%comment" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Comment" ), "filename-comment-amarok", Comment ) );
                i += 8;
            }
            else if( s.mid( i, 6 ) == "%genre" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Genre" ), "filename-genre-amarok", Genre ) );
                i += 6;
            }
            else if( s.mid( i, 9 ) == "%filetype" )
            {
                m_dropTarget->insertToken( new Token( i18n( "File type" ), "filename-filetype-amarok", FileType ) );
                i += 9;
            }
            else if( s.mid( i, 7 ) == "%ignore" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Ignore" ), "filename-ignore-amarok", Ignore ) );
                i += 7;
            }
            else if( s.mid( i, 7 ) == "%folder" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Folder" ), "filename-folder-amarok", Folder ) );
                i += 7;
            }
            else if( s.mid( i, 8 ) == "%initial" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Initial" ), "filename-initial-amarok", Initial ) );
                i += 8;
            }
            else if( s.mid( i, 11 ) == "%discnumber" )
            {
                m_dropTarget->insertToken( new Token( i18n( "Disc number" ), "filename-discnumber-amarok", DiscNumber ) );
                i += 11;
            }
            else
                ++i; // skip junk
        }
        else
        {
            if( s.at(i) == '_' )
                m_dropTarget->insertToken( new Token( "_", "filename-underscore-amarok", Underscore ) );
            else if( s.at(i) == '-' )
                m_dropTarget->insertToken( new Token( "-", "filename-dash-amarok", Dash ) );
            else if( s.at(i) == '.' )
                m_dropTarget->insertToken( new Token( ".", "filename-dot-amarok", Dot ) );
            else if( s.at(i) == ' ' )
                m_dropTarget->insertToken( new Token( " ", "filename-space-amarok", Space ) );
            else if( s.at(i) == '/' )
                m_dropTarget->insertToken( new Token( "/", "filename-slash-amarok", Slash ) );
            else
                debug() << "'" << s.at(i) << "' can't be represented as TokenLayoutWidget Token";
            i++;
        }
    }
}

