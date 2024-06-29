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

#include "TagGuesserDialog.h"
#include "TagGuesser.h"

#include "../widgets/TokenPool.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include "MetaValues.h"
#include "TagsFromFileNameGuesser.h"

#include <QBoxLayout>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#define album_color       Qt::red
#define albumartist_color Qt::blue
#define artist_color      Qt::blue
#define comment_color     Qt::gray
#define composer_color    Qt::magenta
#define genre_color       Qt::cyan
#define title_color       Qt::green
#define track_color       Qt::yellow
#define discnr_color      Qt::yellow
#define year_color        Qt::darkRed


// -------------- TagGuessOptionWidget ------------
TagGuessOptionWidget::TagGuessOptionWidget( QWidget *parent )
    : QWidget( parent )
{
    setupUi( this );

    m_caseEditRadioButtons << rbAllUpper
        << rbAllLower
        << rbFirstLetter
        << rbTitleCase;

    int caseOptions = Amarok::config( QStringLiteral("TagGuesser") ).readEntry( "Case options", 4 );
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

    cbEliminateSpaces->setChecked(    Amarok::config( QStringLiteral("TagGuesser") ).readEntry( "Eliminate trailing spaces", false ) );
    cbReplaceUnderscores->setChecked( Amarok::config( QStringLiteral("TagGuesser") ).readEntry( "Replace underscores", false ) );

    connect( cbCase, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::editStateEnable );
    connect( cbCase, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( rbTitleCase, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( rbFirstLetter, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( rbAllLower, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( rbAllUpper, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( cbEliminateSpaces, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
    connect( cbReplaceUnderscores, &QCheckBox::toggled,
             this, &TagGuessOptionWidget::changed );
}

void
TagGuessOptionWidget::editStateEnable( bool checked )      //SLOT
{
    for( QRadioButton *rb : m_caseEditRadioButtons )
        rb->setEnabled( checked );
}

//Returns a code for the configuration.
int
TagGuessOptionWidget::getCaseOptions()
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
TagGuessOptionWidget::getWhitespaceOptions()
{
    return cbEliminateSpaces->isChecked();
}

//As above
bool
TagGuessOptionWidget::getUnderscoreOptions()
{
    return cbReplaceUnderscores->isChecked();
}


// ------------------------- TagGuesserWidget -------------------

TagGuesserWidget::TagGuesserWidget( QWidget *parent )
    : FilenameLayoutWidget( parent )
{
    m_configCategory = QStringLiteral("FilenameLayoutWidget");

    m_tokenPool->addToken( createToken( Title ) );
    m_tokenPool->addToken( createToken( Artist ) );
    m_tokenPool->addToken( createToken( AlbumArtist ) );
    m_tokenPool->addToken( createToken( Album ) );
    m_tokenPool->addToken( createToken( Genre ) );
    m_tokenPool->addToken( createToken( Composer ) );
    m_tokenPool->addToken( createToken( Comment ) );
    m_tokenPool->addToken( createToken( Year ) );
    m_tokenPool->addToken( createToken( TrackNumber ) );
    m_tokenPool->addToken( createToken( DiscNumber ) );
    m_tokenPool->addToken( createToken( Ignore ) );

    m_tokenPool->addToken( createToken( Slash ) );
    m_tokenPool->addToken( createToken( Underscore ) );
    m_tokenPool->addToken( createToken( Dash ) );
    m_tokenPool->addToken( createToken( Dot ) );
    m_tokenPool->addToken( createToken( Space ) );

    m_syntaxLabel->setText( i18nc("Please do not translate the %foo% words as they define a syntax used internally by a parser to describe a filename.",
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
}

Token*
TagGuesserWidget::createToken(qint64 value) const
{
    Token* token = FilenameLayoutWidget::createToken( value );

    // return colored tokens.
    QColor color = Qt::transparent;
    switch( value )
    {
    case TrackNumber: color = QColor( track_color ); break;
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

// -------------- TagGuesserDialog ------------
TagGuesserDialog::TagGuesserDialog( const QString &fileName, QWidget *parent )
    : QDialog( parent )
    , m_fileName( fileName )
{
    setWindowTitle( i18n( "Guess Tags from Filename" ) );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
    QWidget* mainWidget = new QWidget( this );
    QBoxLayout* mainLayout = new QVBoxLayout(this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TagGuesserDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TagGuesserDialog::reject);

    m_layoutWidget = new TagGuesserWidget( this );
    mainLayout->addWidget( m_layoutWidget );

    m_filenamePreview = new QLabel();
    m_filenamePreview->setAlignment( Qt::AlignHCenter );
    mainLayout->addWidget( m_filenamePreview );

    m_optionsWidget =  new TagGuessOptionWidget();
    mainLayout->addWidget( m_optionsWidget );

    connect( m_layoutWidget, &TagGuesserWidget::schemeChanged,
             this, &TagGuesserDialog::updatePreview );
    connect( m_optionsWidget, &TagGuessOptionWidget::changed,
             this, &TagGuesserDialog::updatePreview );

    updatePreview();

    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);
}

//Sets Filename for Preview
void
TagGuesserDialog::setFileName( const QString& fileName )
{
    m_fileName = fileName;
    updatePreview();
}

//Stores the configuration when the dialog is accepted.
void
TagGuesserDialog::onAccept()    //SLOT
{
    m_layoutWidget->onAccept();

    Amarok::config( QStringLiteral("TagGuesser") ).writeEntry( "Case options", m_optionsWidget->getCaseOptions() );
    Amarok::config( QStringLiteral("TagGuesser") ).writeEntry( "Eliminate trailing spaces", m_optionsWidget->getWhitespaceOptions() );
    Amarok::config( QStringLiteral("TagGuesser") ).writeEntry( "Replace underscores", m_optionsWidget->getUnderscoreOptions() );
}

QMap<qint64,QString>
TagGuesserDialog::guessedTags()
{
    DEBUG_BLOCK;

    QString scheme = m_layoutWidget->getParsableScheme();
    QString fileName = getParsableFileName();

    if( scheme.isEmpty() )
        return QMap<qint64,QString>();

    TagGuesser guesser;
    guesser.setFilename( fileName );
    guesser.setCaseType( m_optionsWidget->getCaseOptions() );
    guesser.setConvertUnderscores( m_optionsWidget->getUnderscoreOptions() );
    guesser.setCutTrailingSpaces( m_optionsWidget->getWhitespaceOptions() );
    guesser.setSchema( scheme );

    if( !guesser.guess() )
    {
        m_filenamePreview->setText( getParsableFileName() );
        return QMap<qint64,QString>();
    }

    return guesser.tags();
}


//Updates the Filename Preview
void
TagGuesserDialog::updatePreview()                 //SLOT
{
    DEBUG_BLOCK;

    QMap<qint64,QString> tags = guessedTags();

    m_filenamePreview->setText( coloredFileName( tags ) );

    QString emptyTagText = i18nc( "Text to represent an empty tag. Braces (<>) are only to clarify emptiness.", "&lt;empty&gt;" );

    quint64 fields[] = {
        Meta::valAlbum,
        Meta::valAlbumArtist,
        Meta::valTitle,
        Meta::valAlbum,
        Meta::valArtist,
        Meta::valComposer,
        Meta::valGenre,
        Meta::valComment,
        Meta::valTrackNr,
        Meta::valYear,
        0};

    QLabel *labels[] = {
        m_optionsWidget->Album_result,
        m_optionsWidget->AlbumArtist_result,
        m_optionsWidget->Title_result,
        m_optionsWidget->Album_result,
        m_optionsWidget->Artist_result,
        m_optionsWidget->Composer_result,
        m_optionsWidget->Genre_result,
        m_optionsWidget->Comment_result,
        m_optionsWidget->Track_result,
        m_optionsWidget->Year_result,
        nullptr};

    for( int i = 0; fields[i]; i++ )
    {
        if( tags.contains( fields[i] ) )
            labels[i]->setText( QStringLiteral("<font color='") + TagGuesserDialog::fieldColor( fields[i] ) + QStringLiteral("'>") + tags[ fields[i] ] + QStringLiteral("</font>") );
        else
            labels[i]->setText( emptyTagText );
    }
}

QString
TagGuesserDialog::parsableFileName( const QFileInfo &fileInfo ) const
{
    DEBUG_BLOCK;
    QString path = fileInfo.absoluteFilePath();

    debug() << m_layoutWidget->getParsableScheme() << "; " << path;

    int schemaLevels = m_layoutWidget->getParsableScheme().count( QLatin1Char('/') );
    int pathLevels   = path.count( QLatin1Char('/') );

    // -- cut paths
    int pos;
    for( pos = 0; pathLevels > schemaLevels && pos < path.length(); pos++ )
        if( path[pos] == QLatin1Char('/') )
            pathLevels--;

    // -- cut extension
    int dotPos = path.lastIndexOf( QLatin1Char('.') );
    if( dotPos >= 0 )
        dotPos -= pos;

debug() << "parsableFileName schemaLevels:" << schemaLevels << "pathLevels:" << pathLevels << "path:" << path << "pos:" << pos << dotPos << path.mid( pos, dotPos );
    return path.mid( pos, dotPos );
}

QString
TagGuesserDialog::getParsableFileName()
{
    return parsableFileName( QFileInfo( m_fileName ) );
}

// creates a colored version of the filename
QString
TagGuesserDialog::coloredFileName( const QMap<qint64,QString> &tags )
{
    QString coloredFileName = m_fileName;

    for( qint64 key : tags.keys() )
    {
        QString value = tags[key];
        // TODO: replace is not the right way to do this.
        coloredFileName.replace( value, QStringLiteral("<font color=\"") + fieldColor( key ) +
                                 QStringLiteral("\">") + value + QStringLiteral("</font>"), Qt::CaseInsensitive );
    }
    return coloredFileName;
}

QString
TagGuesserDialog::fieldColor( qint64 field )
{
    Qt::GlobalColor color;
    switch ( field )
    {
        case Meta::valAlbum:
            color = album_color;
            break;

        case Meta::valAlbumArtist:
            color = albumartist_color;
            break;

        case Meta::valArtist:
            color = artist_color;
            break;

        case Meta::valComment:
            color = comment_color;
            break;

        case Meta::valComposer:
            color = composer_color;
            break;

        case Meta::valDiscNr:
            color = discnr_color;
            break;

        case Meta::valGenre:
            color = genre_color;
            break;

        case Meta::valTitle:
            color = title_color;
            break;

        case Meta::valTrackNr:
            color = track_color;
            break;

        case Meta::valYear:
            color = year_color;
            break;

        default:
            color = Qt::black;
    }

    return QColor( color ).name();
}
