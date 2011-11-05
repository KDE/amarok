/****************************************************************************************
 * Copyright (c) 2006 Giovanni Venturi <giovanni@kde-it.org>                            *
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "EditFilterDialog.h"

#include "ui_EditFilterDialog.h"

#include "amarokconfig.h"

#include "Expression.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "widgets/TokenDropTarget.h"

#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <QPushButton>

#define OR_TOKEN Meta::valCustom  + 1
#define AND_TOKEN Meta::valCustom + 2

#define AND_TOKEN_CONSTRUCT new Token( i18n( "AND" ), "filename-and-amarok", AND_TOKEN )
#define OR_TOKEN_CONSTRUCT new Token( i18n( "OR" ), "filename-divider", OR_TOKEN )
#define SIMPLE_TEXT_CONSTRUCT new Token( i18n( "Simple text" ), "media-track-edit-amarok", 0 )

EditFilterDialog::EditFilterDialog( QWidget* parent, const QString &text )
    : KDialog( parent )
    , m_ui( new Ui::EditFilterDialog )
    , m_curToken( 0 )
    , m_separator( " AND " )
{
    setCaption( i18n( "Edit Filter" ) );
    setButtons( KDialog::Reset | KDialog::Ok | KDialog::Cancel );

    m_ui->setupUi( mainWidget() );
    setMinimumSize( minimumSizeHint() );

    m_dropTarget = new TokenDropTarget( "application/x-amarok-tag-token", m_ui->dtTokens );
    m_dropTarget->setRowLimit( 1 );
    m_dropTarget->layout()->setContentsMargins( 1, 1, 1, 1 );

    QVBoxLayout *l = new QVBoxLayout( m_ui->dtTokens );
    l->setContentsMargins( 0, 0, 0, 0 );
    l->addWidget( m_dropTarget );

    initTokenPool();
    parseTextFilter( text );
    updateMetaQueryWidgetView();

    connect( m_ui->mqwAttributeEditor, SIGNAL( changed( const MetaQueryWidget::Filter & ) ),
             SLOT( slotAttributeChanged( const MetaQueryWidget::Filter & ) ) );
    connect( this, SIGNAL( resetClicked() ), SLOT( slotReset() ) );
    connect( m_ui->cbInvert, SIGNAL( toggled( bool ) ),
             SLOT( slotInvert( bool ) ) );
    connect( m_ui->cbAndOr, SIGNAL( currentIndexChanged( int ) ),
             SLOT( slotSeparatorChange( int ) ) );
    connect( m_dropTarget, SIGNAL( focusReceived( QWidget * ) ),
             SLOT( slotTokenSelected( QWidget * ) ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             SLOT( slotTokenDropTargetChanged() ) );
}

EditFilterDialog::~EditFilterDialog()
{
    delete m_ui;
}

void
EditFilterDialog::initTokenPool()
{

    m_ui->tpTokenPool->addToken( SIMPLE_TEXT_CONSTRUCT );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valTitle ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valArtist ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valAlbumArtist ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valAlbum ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valGenre ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valComposer ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valComment ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valUrl ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valYear ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valTrackNr ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valDiscNr ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valBpm ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valLength ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valBitrate ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valSamplerate ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valFilesize ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valFormat ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valCreateDate ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valScore ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valRating ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valFirstPlayed ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valPlaycount ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valLabel ) );
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valModified ) );
    m_ui->tpTokenPool->addToken( OR_TOKEN_CONSTRUCT );
    m_ui->tpTokenPool->addToken( AND_TOKEN_CONSTRUCT );
}

Token *
EditFilterDialog::tokenForField( const qint64 field )
{
    QString icon = Meta::iconForField( field );
    QString text = Meta::i18nForField( field );

    return new Token( text, icon, field );
}

void
EditFilterDialog::slotAttributeChanged( const MetaQueryWidget::Filter &filter )
{
    if( m_curToken )
        m_filters[m_curToken].filter = filter;

    m_ui->label->setText( this->filter() );
}

void
EditFilterDialog::slotInvert( bool checked )
{
    if( m_curToken )
        m_filters[m_curToken].inverted = checked;

    m_ui->label->setText( filter() );
}

void
EditFilterDialog::slotSeparatorChange( int index )
{
    // this depends on the order of combobox entries in EditFilterDialog.ui
    // but fixes Bug 279559
    if( index == 0 )
        m_separator = QString( " AND " );
    else
        m_separator = QString( " OR " );

    m_ui->label->setText( filter() );
}

void
EditFilterDialog::slotReset()
{
    m_curToken = 0;
    m_filters.clear();
    m_dropTarget->clear();
    m_ui->cbAndOr->setCurrentIndex( 0 );

    updateMetaQueryWidgetView();
}

void
EditFilterDialog::accept()
{
    emit filterChanged( filter() );
    KDialog::accept();
}

void
EditFilterDialog::updateMetaQueryWidgetView()
{
    if( m_curToken )
    {
        if( m_filters.contains( m_curToken ) )
        {
            m_ui->mqwAttributeEditor->setFilter( m_filters[m_curToken].filter );
            m_ui->cbInvert->setChecked( m_filters[m_curToken].inverted );
        }
        else
        {
            m_ui->mqwAttributeEditor->setField( m_curToken->value() );
            m_ui->cbInvert->setChecked( false );
        }
    }
    else
    {
        m_ui->mqwAttributeEditor->setField( 0 );
        m_ui->cbInvert->setChecked( false );
    }

    m_ui->mqwAttributeEditor->setEnabled( ( bool )m_curToken );
    m_ui->cbInvert->setEnabled( ( bool )m_curToken );
    m_ui->label->setText( filter() );
}

void
EditFilterDialog::slotTokenSelected( QWidget *token )
{
    m_curToken = qobject_cast< Token *>( token );

    if( m_curToken && m_curToken->value() > Meta::valCustom )   // OR / AND tokens case
        m_curToken = 0;

    updateMetaQueryWidgetView();
}

void
EditFilterDialog::slotTokenDropTargetChanged()
{
    m_curToken = 0;
    updateMetaQueryWidgetView();
}

QString
EditFilterDialog::filter() const
{
    QString filterString;

    if( !m_dropTarget->count() )
        return filterString;

    QList < Token *> tokens = m_dropTarget->drags();
    bool join = false;
    Filter filter;
    foreach( Token *token, tokens )
    {
        if( token->value() == OR_TOKEN )
        {
            filterString.append( " OR " );
            join = false;
        }
        else if( token->value() == AND_TOKEN )
        {
            filterString.append( " AND " );
            join = false;
        }
        else if( m_filters.contains( token ) )
        {
            if( join )
                filterString.append( m_separator );
            filter = m_filters[token];
            filterString.append( filter.filter.toString( filter.inverted ) );
            join = true;
        }
    }

    return filterString;
}

void
EditFilterDialog::parseTextFilter( const QString &text )
{
    ParsedExpression parsed = ExpressionParser::parse ( text );
    bool AND = false;
    bool OR = false;
    foreach( const or_list &orList, parsed )
    {
        if( AND )
            m_dropTarget->insertToken( AND_TOKEN_CONSTRUCT );

        OR = false;
        foreach ( const expression_element &elem, orList )
        {
            if( OR )
                m_dropTarget->insertToken( OR_TOKEN_CONSTRUCT );

            Filter filter;
            filter.filter.field = !elem.field.isEmpty() ? Meta::fieldForName( elem.field ) : 0;
            if( filter.filter.field == Meta::valRating )
                filter.filter.numValue = 2 * elem.text.toFloat();
            else if( m_ui->mqwAttributeEditor->isDate( filter.filter.field ) )
            {
                quint64 today = QDateTime::currentDateTime().toTime_t();
                bool invert = elem.text.startsWith( '-' );
                QString strTime = elem.text.mid( invert, elem.text.length() - 1 - invert );
                quint64 diff = strTime.toULongLong();
                switch( elem.text[elem.text.length() - 1].toAscii() )
                {
                    case 'd':
                        diff *= 24;
                    case 'h':
                        diff *= 60;
                    case 'M':
                        diff *= 60;
                }

                filter.filter.numValue = today - ( invert ? -diff : diff );
            }
            else if( m_ui->mqwAttributeEditor->isNumeric( filter.filter.field ) )
                filter.filter.numValue = elem.text.toInt();

            if( m_ui->mqwAttributeEditor->isNumeric( filter.filter.field ) )
            {
                switch( elem.match )
                {
                    case expression_element::Equals:
                        filter.filter.condition = MetaQueryWidget::Equals;
                        break;
                    case expression_element::Less:
                        filter.filter.condition = MetaQueryWidget::LessThan;
                        break;
                    case expression_element::More:
                        filter.filter.condition = MetaQueryWidget::GreaterThan;
                        break;
                }
            }
            else
            {
                switch( elem.match )
                {
                    case expression_element::Contains:
                        filter.filter.condition = MetaQueryWidget::Contains;
                        break;
                    case expression_element::Equals:
                        filter.filter.condition = MetaQueryWidget::Equals;
                        break;
                }
                filter.filter.value = elem.text;
            }

            filter.inverted = elem.negate;

            Token *nToken = filter.filter.field
                            ? tokenForField( filter.filter.field )
                            : SIMPLE_TEXT_CONSTRUCT;
            m_dropTarget->insertToken( nToken );
            m_filters.insert( nToken, filter );

            OR = true;
        }

        AND = true;
    }
}

#include "EditFilterDialog.moc"

