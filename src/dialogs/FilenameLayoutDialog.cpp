/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time.shift.de>                       *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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
#include "TagGuesser.h"

#include "amarokconfig.h"

#include "MetaValues.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "widgets/TokenDropTarget.h"

#include <KConfig>
#include <KColorScheme>
#include <KInputDialog>

static const QStringList typeElements = ( QStringList()
<< QString()
<< QLatin1String("%ignore%")
<< QLatin1String("%track%")
<< QLatin1String("%title%")
<< QLatin1String("%artist%")
<< QLatin1String("%composer%")
<< QLatin1String("%year%")
<< QLatin1String("%album%")
<< QLatin1String("%albumartist%")
<< QLatin1String("%comment%")
<< QLatin1String("%genre%")
<< QLatin1String("%filetype%")
<< QLatin1String("%folder%")
<< QLatin1String("%initial%")
<< QLatin1String("%discnumber%")
<< QLatin1String(" ")
<< QLatin1String("/")
<< QLatin1String(".")
<< QLatin1String("-")
<< QLatin1String("_") );


// ------------------------- OrganizeCollectionWidget -------------------

OrganizeCollectionWidget::OrganizeCollectionWidget( QWidget *parent )
    : FilenameLayoutWidget( parent )
{
    m_configCategory = "OrganizeCollectionDialog";

    optionsFrame->hide();
    resultGroupBox->hide();
    filenamePreview->hide();

    tokenPool->addToken( createToken( Track ) );
    tokenPool->addToken( createToken( Title ) );
    tokenPool->addToken( createToken( Artist ) );
    tokenPool->addToken( createToken( Composer ) );
    tokenPool->addToken( createToken( Year ) );
    tokenPool->addToken( createToken( Album ) );
    tokenPool->addToken( createToken( AlbumArtist ) );
    tokenPool->addToken( createToken( Comment ) );
    tokenPool->addToken( createToken( Genre ) );

    tokenPool->addToken( createToken( Initial ) );
    tokenPool->addToken( createToken( FileType ) );
    tokenPool->addToken( createToken( DiscNumber ) );

    tokenPool->addToken( createToken( Slash ) );
    tokenPool->addToken( createToken( Underscore ) );
    tokenPool->addToken( createToken( Dash ) );
    tokenPool->addToken( createToken( Dot ) );
    tokenPool->addToken( createToken( Space ) );

    //INIT for collection root
    unsigned int borderColor = static_cast<unsigned int>( KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color().rgb() );
    collectionRootFrame->setStyleSheet( "\
            color: palette( Text );\
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
            color: palette( Text );\
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

    dotFrame->setStyleSheet( "\
            color: palette( Text );\
            border: 2px solid #" + QString::number( borderColor, 16 ).remove( 0, 2 ) + ";\
            border-radius: 4px;\
            padding: 2px;\
            " );

    QHBoxLayout *dotLayout = new QHBoxLayout( dotFrame );
    QLabel *dotIconLabel = new QLabel( "", this );
    QLabel *dotLabel = new QLabel( "." , this );
    dotLayout->addWidget( dotIconLabel );
    dotLayout->addWidget( dotLabel );
    dotLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
    dotIconLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
    dotLayout->setContentsMargins( 0, 0, 0, 0 );
    dotIconLabel->setContentsMargins( 0, 0, 0, 0 );
    dotLabel->setContentsMargins( 0, 0, 0, 0 );
    dotIconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    dotIconLabel->setFixedSize( 16, 16 );
    QPixmap dotIcon = QPixmap( KIcon( "filename-dot-amarok" ).pixmap(16, 16) );
    dotIconLabel->setPixmap( dotIcon );


    extensionFrame->setStyleSheet( "\
            color: palette( Text );\
            border: 2px solid #" + QString::number( borderColor, 16 ).remove( 0, 2 ) + ";\
            border-radius: 4px;\
            padding: 2px;\
            " );

    QHBoxLayout *extensionLayout = new QHBoxLayout( extensionFrame );
    QLabel *extensionIconLabel = new QLabel( "", this );
    QLabel *extensionLabel = new QLabel( i18n("File Type"), this );
    extensionLayout->addWidget( extensionIconLabel );
    extensionLayout->addWidget( extensionLabel );
    extensionLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
    extensionIconLabel->setStyleSheet( "border:0px solid #000000; border-radius: 0px; padding: 0px;" );
    extensionLayout->setContentsMargins( 0, 0, 0, 0 );
    extensionIconLabel->setContentsMargins( 0, 0, 0, 0 );
    extensionLabel->setContentsMargins( 0, 0, 0, 0 );
    extensionIconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    extensionIconLabel->setFixedSize( 16, 16 );
    QPixmap extensionIcon = QPixmap( KIcon( "filename-filetype-amarok" ).pixmap(16, 16) );
    extensionIconLabel->setPixmap( extensionIcon );

    syntaxLabel->setText( i18nc("Please do not translate the %foo% words as they define a syntax used internally by a parser to describe a filename.",
                          // xgettext: no-c-format
                          "The following tokens can be used to define a filename scheme: \
                          <br>%track%, %title%, %artist%, %composer%, %year%, %album%, %albumartist%, %comment%, %genre%, %initial%, %folder%, %filetype%, %discnumber%." ) );

    populateConfiguration();
}

// ------------------------- TagGuesserWidget -------------------

TagGuesserWidget::TagGuesserWidget( QWidget *parent )
    : FilenameLayoutWidget( parent )
{
    m_configCategory = "FilenameLayoutWidget";

    optionsFrame->show();
    resultGroupBox->show();
    filenamePreview->show();

    tokenPool->addToken( createToken( Track ) );
    tokenPool->addToken( createToken( Title ) );
    tokenPool->addToken( createToken( Artist ) );
    tokenPool->addToken( createToken( Composer ) );
    tokenPool->addToken( createToken( Year ) );
    tokenPool->addToken( createToken( Album ) );
    tokenPool->addToken( createToken( AlbumArtist ) );
    tokenPool->addToken( createToken( Comment ) );
    tokenPool->addToken( createToken( Genre ) );
    tokenPool->addToken( createToken( Ignore ) );
    tokenPool->addToken( createToken( Slash ) );
    tokenPool->addToken( createToken( Underscore ) );
    tokenPool->addToken( createToken( Dash ) );
    tokenPool->addToken( createToken( Dot ) );
    tokenPool->addToken( createToken( Space ) );

    int caseOptions = Amarok::config( "TagGuesser" ).readEntry( "Case options", 4 );
    if( !caseOptions )
        cbCase->setChecked( false );
    else
    {
        cbCase->setChecked( true );
        switch( caseOptions )
        {
            case 4:
                rbAllLower->setChecked( true );
                break;
            case 3:
                rbAllUpper->setChecked( true );
                break;
            case 2:
                rbFirstLetter->setChecked( true );
                break;
            case 1:
                rbTitleCase->setChecked( true );
                break;
            default:
                debug() << "OUCH";
        }
    }

    cbEliminateSpaces->setChecked(    Amarok::config( "TagGuesser" ).readEntry( "Eliminate trailing spaces", false ) );
    cbReplaceUnderscores->setChecked( Amarok::config( "TagGuesser" ).readEntry( "Replace underscores", false ) );
    cbUseFullPath->setChecked(        Amarok::config( "TagGuesser" ).readEntry( "Use full file path", false ) );
    sbNestingLevel->setValue(         Amarok::config( "TagGuesser" ).readEntry( "Directories nesting level", 0 ) );

    syntaxLabel->setText( i18nc("Please do not translate the %foo% words as they define a syntax used internally by a parser to describe a filename.",
                          // xgettext: no-c-format
                          "The following tokens can be used to define a filename scheme:<br> \
                          <font color=\"%1\">%track%</font>, <font color=\"%2\">%title%</font>, \
                          <font color=\"%3\">%artist%</font>, <font color=\"%4\">%composer%</font>, \
                          <font color=\"%5\">%year%</font>, <font color=\"%6\">%album%</font>, \
                          <font color=\"%7\">%albumartist%</font>, <font color=\"%8\">%comment%</font>, \
                          <font color=\"%9\">%genre%</font>, %ignore%."
                          , QColor( track_color ).name(), QColor( title_color ).name(), QColor( artist_color ).name(), \
                          QColor( composer_color ).name(), QColor( year_color ).name(), QColor( album_color ).name(), QColor( albumartist_color ).name(), \
                          QColor( comment_color ).name(), QColor( genre_color ).name() ) );

    populateConfiguration();

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( updatePreview() ) );
    connect( rbTitleCase, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( rbFirstLetter, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( rbAllLower, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( rbAllUpper, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( cbEliminateSpaces, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( cbReplaceUnderscores, SIGNAL( toggled(bool) ),
             this, SLOT( updatePreview() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             this, SLOT( updatePreview() ) );
    connect( filenameLayoutEdit, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( updatePreview() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             this, SLOT( updatePreview() ) );
    connect( cbUseFullPath, SIGNAL( toggled( bool ) ),
             this, SLOT( updatePreview() ) );
    connect( sbNestingLevel, SIGNAL( valueChanged( int ) ),
             this, SLOT( updatePreview() ) );
}

//Sets Filename for Preview
void
TagGuesserWidget::setFileName( const QString& fileName )
{
    m_filename = fileName;
    sbNestingLevel->setMaximum( fileName.count( '/' ) - 1 );
    updatePreview();
}

QString
TagGuesserWidget::getParsableFileName()
{
    return parsableFileName( QFileInfo( m_filename ) );
}

//Stores the configuration when the dialog is accepted.
void
TagGuesserWidget::onAccept()    //SLOT
{
    FilenameLayoutWidget::onAccept();

    Amarok::config( "TagGuesser" ).writeEntry( "Case options", getCaseOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Eliminate trailing spaces", getWhitespaceOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Replace underscores", getUnderscoreOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Use full file path", cbUseFullPath->isChecked() );
    Amarok::config( "TagGuesser" ).writeEntry( "Directories nesting level", sbNestingLevel->value() );
}

//Updates the Filename Preview
void
TagGuesserWidget::updatePreview()                 //SLOT
{
    DEBUG_BLOCK;

    QString scheme = this->getParsableScheme();
    QString fileName = getParsableFileName();

    if( scheme.isEmpty() )
    {
        filenamePreview->setText( fileName );
        return;
    }

    TagGuesser guesser;
    guesser.setFilename( fileName );
    guesser.setCaseType( getCaseOptions() );
    guesser.setConvertUnderscores( getUnderscoreOptions() );
    guesser.setCutTrailingSpaces( getWhitespaceOptions() );
    guesser.setSchema( scheme );

    if( !guesser.guess() )
    {
        filenamePreview->setText( fileName );
        return;
    }

    QMap<qint64,QString> tags = guesser.tags();
    QString emptyTagText = i18nc( "Text to represent an empty tag. Braces (<>) are only to clarify emptiness.", "&lt;empty&gt;" );

    if( tags.contains( Meta::valAlbum ) )
        Album_result->setText( "<font color='" + QColor( album_color ).name() + "'>" + tags[Meta::valAlbum] + "</font>" );
    else
        Album_result->setText( emptyTagText );

    if( tags.contains( Meta::valAlbumArtist ) )
        AlbumArtist_result->setText( "<font color='" + QColor( albumartist_color ).name() + "'>" + tags[Meta::valAlbumArtist] + "</font>" );
    else
        AlbumArtist_result->setText( emptyTagText );

    if( tags.contains( Meta::valTitle ) )
        Title_result->setText( "<font color='" + QColor( title_color ).name() + "'>" + tags[Meta::valTitle] + "</font>" );
    else
        Title_result->setText( emptyTagText );

    if( tags.contains( Meta::valArtist ) )
        Artist_result->setText( "<font color='" + QColor( artist_color ).name() + "'>" + tags[Meta::valArtist] + "</font>" );
    else
        Artist_result->setText( emptyTagText );

    if( tags.contains( Meta::valComment ) )
        Comment_result->setText( "<font color='" + QColor( comment_color ).name() + "'>" + tags[Meta::valComment] + "</font>" );
    else
        Comment_result->setText( emptyTagText );

    if( tags.contains( Meta::valComposer ) )
        Composer_result->setText( "<font color='" + QColor( composer_color ).name() + "'>" + tags[Meta::valComposer] + "</font>" );
    else
        Composer_result->setText( emptyTagText );

    if( tags.contains( Meta::valGenre ) )
        Genre_result->setText( "<font color='" + QColor( genre_color ).name() + "'>" + tags[Meta::valGenre] + "</font>" );
    else
        Genre_result->setText( emptyTagText );

    if( tags.contains( Meta::valTrackNr ) )
        Track_result->setText( "<font color='" + QColor( track_color ).name() + "'>" + tags[Meta::valTrackNr] + "</font>" );
    else
        Track_result->setText( emptyTagText );

    if( tags.contains( Meta::valYear ) )
        Year_result->setText( "<font color='" + QColor( year_color ).name() + "'>" + tags[Meta::valYear] + "</font>" );
    else
        Year_result->setText( emptyTagText );

    filenamePreview->setText(guesser.coloredFileName());
}

//Returns a code for the configuration.
int
TagGuesserWidget::getCaseOptions()
{
    //Amarok::config( "TagGuesser" ).readEntry( "Filename schemes", QStringList() );
    if( !cbCase->isChecked() )
        return 0;
    else
    {
        if( rbAllLower->isChecked() )
            return 4;
        else if( rbAllUpper->isChecked() )
            return 3;
        else if( rbFirstLetter->isChecked() )
            return 2;
        else if( rbTitleCase->isChecked() )
            return 1;
        else
        {
            debug() << "OUCH!";
            return 0;
        }
    }
}

//As above
bool
TagGuesserWidget::getWhitespaceOptions()
{
    return cbEliminateSpaces->isChecked();
}

//As above
bool
TagGuesserWidget::getUnderscoreOptions()
{
    return cbReplaceUnderscores->isChecked();
}

Token*
TagGuesserWidget::createToken(qint64 value) const
{
    Token* token = FilenameLayoutWidget::createToken( value );

    // return colored tokens.
    QColor color = Qt::transparent;
    switch( value )
    {
    case Track: color = QColor( track_color ); break;
    case Title: color = QColor( title_color ); break;
    case Artist: color = QColor( artist_color ); break;
    case Composer: color = QColor( composer_color ); break;
    case Year: color = QColor( year_color ); break;
    case Album: color = QColor( album_color ); break;
    case AlbumArtist: color = QColor( albumartist_color ); break;
    case Comment: color = QColor( comment_color ); break;
    case Genre: color = QColor( genre_color );
    }
    if (color != Qt::transparent)
        token->setTextColor( color );

    return token;
}

QString
TagGuesserWidget::parsableFileName( const QFileInfo &fileInfo ) const
{
    if( !cbUseFullPath->isChecked() || !sbNestingLevel->value() )
        return fileInfo.fileName();

    QString path = fileInfo.absoluteFilePath();
    int pos, n = sbNestingLevel->value() + 1;

    for( pos = 0; pos < path.length() && n; pos++ )
        if( path[pos] == '/' )
            n--;

    return path.mid( pos );
}




// ------------------------- FilenameLayoutWidget -------------------

FilenameLayoutWidget::FilenameLayoutWidget( QWidget *parent )
    : QWidget( parent )
    , m_advancedMode( false )
{
    setupUi( this );

    m_caseEditRadioButtons << rbAllUpper << rbAllLower << rbFirstLetter << rbTitleCase;

    m_dropTarget = new TokenDropTarget( "application/x-amarok-tag-token", filenameLayout );
    m_dropTarget->setRowLimit( 1 );

    QVBoxLayout *l = new QVBoxLayout(filenameLayout);
    l->setContentsMargins( 0, 0, 0, 0 );
    l->addWidget(m_dropTarget);

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( editStateEnable( bool ) ) );
    connect( tokenPool, SIGNAL( onDoubleClick( Token * ) ),
             m_dropTarget, SLOT( insertToken( Token* ) ) );
    connect( kpbAdvanced, SIGNAL( clicked() ),
             this, SLOT( toggleAdvancedMode() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             this, SIGNAL( schemeChanged() ) );
    connect( addPresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotAddFormat() ) );
    connect( removePresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotRemoveFormat() ) );
    connect( updatePresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotUpdateFormat() ) );

    connect( filenameLayoutEdit, SIGNAL( textChanged( const QString & ) ),
             this, SIGNAL( schemeChanged() ) );
    connect( spaceCheck, SIGNAL(toggled(bool)), SIGNAL(schemeChanged()) );
    connect( ignoreTheCheck, SIGNAL(toggled(bool)), SIGNAL(schemeChanged()) );
    connect( vfatCheck, SIGNAL(toggled(bool)), SIGNAL(schemeChanged()) );
    connect( asciiCheck, SIGNAL(toggled(bool)), SIGNAL(schemeChanged()) );
    connect( regexpEdit, SIGNAL(editingFinished()), SIGNAL(schemeChanged()) );
    connect( replaceEdit, SIGNAL(editingFinished()), SIGNAL(schemeChanged()) );
}

Token*
FilenameLayoutWidget::createToken(qint64 value) const
{
    struct TokenDefinition
    {
        QString name;
        const char* iconName;
        qint64 value;
    };

    static const TokenDefinition tokenDefinitions[] = {
        { i18n( "Track number" ),"filename-track-amarok", Track },
        { i18n( "Title" ), "filename-title-amarok", Title },
        { i18n( "Artist" ), "filename-artist-amarok", Artist },
        { i18n( "Composer" ), "filename-composer-amarok", Composer },
        { i18n( "Year" ), "filename-year-amarok", Year },
        { i18n( "Album" ), "filename-album-amarok", Album },
        { i18n( "Album Artist" ), "filename-artist-amarok", AlbumArtist },
        { i18n( "Comment" ), "filename-comment-amarok", Comment },
        { i18n( "Genre" ), "filename-genre-amarok", Genre },

        { i18n( "File type" ), "filename-filetype-amarok", FileType },
        { i18n( "Ignore" ), "filename-ignore-amarok", Ignore },
        { i18n( "Folder" ), "filename-folder-amarok", Folder },
        { i18nc( "Artist's Initial", "Initial" ), "filename-initial-amarok", Initial },
        { i18n( "Disc number" ), "filename-discnumber-amarok", DiscNumber },

        { "/", "filename-slash-amarok", Slash },
        { "_", "filename-underscore-amarok", Underscore },
        { "-", "filename-dash-amarok", Dash },
        { ".", "filename-dot-amarok", Dot },
        { " ", "filename-space-amarok", Space },
        { QString(), 0, Space }
    };

    for( int i = 0; !tokenDefinitions[i].name.isNull(); ++i )
    {
        if( value == tokenDefinitions[i].value )
        {
            return new Token( tokenDefinitions[i].name,
                              tokenDefinitions[i].iconName,
                              tokenDefinitions[i].value );
        }
    }

    return 0;
}

//Stores the configuration when the dialog is accepted.
void
FilenameLayoutWidget::onAccept()    //SLOT
{
    slotSaveFormatList();
}

//Forwards the request for a scheme to TokenLayoutWidget
QString
FilenameLayoutWidget::getParsableScheme()
{
    QString scheme   = m_advancedMode ? filenameLayoutEdit->text() : parsableScheme();

    Amarok::config( m_configCategory ).writeEntry( "Custom Scheme", scheme );
    return scheme;
}

// attempts to set the scheme
void FilenameLayoutWidget::setScheme(const QString& scheme)
{
    if( m_advancedMode )
        filenameLayoutEdit->setText( scheme );
    else
        inferScheme( scheme );

    emit schemeChanged();
}



//Handles the radiobuttons
void
FilenameLayoutWidget::editStateEnable( bool checked )      //SLOT
{
    foreach( QRadioButton *rb, m_caseEditRadioButtons )
        rb->setEnabled( checked );
}

//Handles the modifications to the dialog to toggle between advanced and basic editing mode.
void
FilenameLayoutWidget::toggleAdvancedMode()
{
    setAdvancedMode( !m_advancedMode );
}

//handles switching between basic and advanced mode
void
FilenameLayoutWidget::setAdvancedMode( bool isAdvanced )
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

    QString entryValue  = m_advancedMode ? "Advanced" : "Basic";

    Amarok::config( m_configCategory ).writeEntry( "Mode", entryValue );
}



// Iterates over the elements of the TokenLayoutWidget bar
// (really over the elements of a QList that stores the indexes
// of the tokens) and generates a string that TagGuesser can digest.
QString
FilenameLayoutWidget::parsableScheme() const
{
    QString parsableScheme = "";

    QList< Token *> list = m_dropTarget->drags( 0 );

    foreach( Token *token, list )
    {
        parsableScheme += typeElements[token->value()];
    }

    return parsableScheme;
}



//tries to populate the widget with tokens according to a string
void
FilenameLayoutWidget::inferScheme( const QString &s ) //SLOT
{
    DEBUG_BLOCK

    debug() << "infering scheme: " << s;

    m_dropTarget->clear();
    for( int i = 0; i < s.size(); )
    {
        if( s.at(i) == '%')
        {
            if( s.midRef( i, 7 ) == "%title%" )
            {
                m_dropTarget->insertToken( createToken( Title ) );
                i += 7;
            }
            else if( s.midRef( i, 7 ) == "%track%" )
            {
                m_dropTarget->insertToken( createToken( Track ) );
                i += 7;
            }
            else if( s.midRef( i, 8 ) == "%artist%" )
            {
                m_dropTarget->insertToken( createToken( Artist ) );
                i += 8;
            }
            else if( s.midRef( i, 10 ) == "%composer%" )
            {
                m_dropTarget->insertToken( createToken( Composer ) );
                i += 10;
            }
            else if( s.midRef( i, 6 ) == "%year%" )
            {
                m_dropTarget->insertToken( createToken( Year ) );
                i += 6;
            }
            else if( s.midRef( i, 13 ) == "%albumartist%" )
            {
                m_dropTarget->insertToken( createToken( AlbumArtist ) );
                i += 13;
            }
            else if( s.midRef( i, 7 ) == "%album%" )
            {
                m_dropTarget->insertToken( createToken( Album ) );
                i += 7;
            }
            else if( s.midRef( i, 9 ) == "%comment%" )
            {
                m_dropTarget->insertToken( createToken( Comment ) );
                i += 9;
            }
            else if( s.midRef( i, 7 ) == "%genre%" )
            {
                m_dropTarget->insertToken( createToken( Genre ) );
                i += 7;
            }
            else if( s.midRef( i, 10 ) == "%filetype%" )
            {
                m_dropTarget->insertToken( createToken( FileType ) );
                i += 10;
            }
            else if( s.midRef( i, 8 ) == "%ignore%" )
            {
                m_dropTarget->insertToken( createToken( Ignore ) );
                i += 8;
            }
            else if( s.midRef( i, 8 ) == "%folder%" )
            {
                m_dropTarget->insertToken( createToken( Folder ) );
                i += 8;
            }
            else if( s.midRef( i, 9 ) == "%initial%" )
            {
                m_dropTarget->insertToken( createToken( Initial ) );
                i += 9;
            }
            else if( s.midRef( i, 12 ) == "%discnumber%" )
            {
                m_dropTarget->insertToken( createToken( DiscNumber ) );
                i += 12;
            }
            else
                ++i; // skip junk
        }
        else
        {
            if( s.at(i) == '_' )
                m_dropTarget->insertToken( createToken( Underscore ) );
            else if( s.at(i) == '-' )
                m_dropTarget->insertToken( createToken( Dash ) );
            else if( s.at(i) == '.' )
                m_dropTarget->insertToken( createToken( Dot ) );
            else if( s.at(i) == ' ' )
                m_dropTarget->insertToken( createToken( Space ) );
            else if( s.at(i) == '/' )
                m_dropTarget->insertToken( createToken( Slash ) );
            else
                debug() << "'" << s.at(i) << "' can't be represented as TokenLayoutWidget Token";
            i++;
        }
    }
}

void
FilenameLayoutWidget::populateConfiguration()
{
    QString mode = Amarok::config( m_configCategory ).readEntry( "Mode" );
    if( mode == QLatin1String( "Advanced" ) )
    {
        setAdvancedMode( true );
        filenameLayoutEdit->setText( Amarok::config( m_configCategory ).readEntryUntranslated( "Custom Scheme" ) );
    }
    else
    {
        setAdvancedMode( false );
        inferScheme( Amarok::config( m_configCategory ).readEntryUntranslated( "Custom Scheme" ) );
    }

    populateFormatList();
}


void
FilenameLayoutWidget::populateFormatList()
{
    DEBUG_BLOCK

    // items are stored in the config list in the following format:
    // Label#DELIM#format string#DELIM#selected
    // the last item to have the third parameter is the default selected preset
    // the third param isnis optional
    QStringList presets_raw;
    int selected_index = -1;
    presetCombo->clear();
    presets_raw = AmarokConfig::formatPresets();
    // presets_raw = Amarok::config( m_configCategory ).readEntry( QString::fromLatin1( "Format Presets" ), QStringList() );

    debug() << "--- got preset for" << m_configCategory << presets_raw;

    foreach( QString str, presets_raw )
    {
        QStringList items;
        items = str.split( "#DELIM#", QString::SkipEmptyParts );
        if( items.size() < 2 )
            continue;
        presetCombo->addItem( items.at( 0 ), items.at( 1 ) ); // Label, format string
        if( items.size() == 3 )
            selected_index = presetCombo->findData( items.at( 1 ) );
    }

    if( selected_index > 0 )
        presetCombo->setCurrentIndex( selected_index );

    slotFormatPresetSelected( selected_index );
    connect( presetCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( slotFormatPresetSelected( int ) ) );
}

void
FilenameLayoutWidget::slotSaveFormatList()
{
    if( !m_formatListModified )
        return;

    QStringList presets;
    int n = presetCombo->count();
    int current_idx = presetCombo->currentIndex();

    for( int i = 0; i < n; ++i )
    {
        QString item;
        if( i == current_idx )
            item = "%1#DELIM#%2#DELIM#selected";
        else
            item = "%1#DELIM#%2";

        QString scheme = presetCombo->itemData( i ).toString();
        QString label = presetCombo->itemText( i );
        item = item.arg( label, scheme );
        presets.append( item );
    }

   Amarok::config( m_configCategory ).writeEntry( QString::fromLatin1( "Format Presets" ), presets );
}

void
FilenameLayoutWidget::slotFormatPresetSelected( int index )
{
    QString scheme = presetCombo->itemData( index ).toString();
    setScheme( scheme );
}

void
FilenameLayoutWidget::slotAddFormat()
{
    bool ok = false;
    QString name = KInputDialog::getText( i18n( "New Format Preset" ), i18n( "Preset Name" ), i18n( "New Preset" ),  &ok, this );
    if( !ok )
        return; // user canceled.

    QString format = getParsableScheme();
    presetCombo->insertItem(0, name, format);
    presetCombo->setCurrentIndex( 0 );
    m_formatListModified = true;
}

void
FilenameLayoutWidget::slotRemoveFormat()
{
    int idx = presetCombo->currentIndex();
    presetCombo->removeItem( idx );
    m_formatListModified = true;
}

void
FilenameLayoutWidget::slotUpdateFormat()
{
    int idx = presetCombo->currentIndex();
    QString formatString = getParsableScheme();
    presetCombo->setItemData( idx, formatString );
    updatePresetButton->setEnabled( false );
    m_formatListModified = true;
}
