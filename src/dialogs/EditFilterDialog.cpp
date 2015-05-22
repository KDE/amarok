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

#include "amarokconfig.h"
#include "ui_EditFilterDialog.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/Expression.h"
#include "dialogs/EditFilterDialog.h"
#include "widgets/TokenDropTarget.h"

#include <KGlobal>
#include <KLocale>
#include <klocalizeddate.h>
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
    , m_isUpdating()
{
    setCaption( i18n( "Edit Filter" ) );
    setButtons( KDialog::Reset | KDialog::Ok | KDialog::Cancel );

    m_ui->setupUi( mainWidget() );

    m_ui->dropTarget->setRowLimit( 1 );

    initTokenPool();

    m_ui->searchEdit->setText( text );
    updateDropTarget( text );
    updateAttributeEditor();

    connect( m_ui->mqwAttributeEditor, SIGNAL(changed(MetaQueryWidget::Filter)),
             SLOT(slotAttributeChanged(MetaQueryWidget::Filter)) );
    connect( this, SIGNAL(resetClicked()), SLOT(slotReset()) );
    connect( m_ui->cbInvert, SIGNAL(toggled(bool)),
             SLOT(slotInvert(bool)) );
    connect( m_ui->rbAnd, SIGNAL(toggled(bool)),
             SLOT(slotSeparatorChange()) );
    connect( m_ui->rbOr, SIGNAL(toggled(bool)),
             SLOT(slotSeparatorChange()) );
    connect( m_ui->tpTokenPool, SIGNAL(onDoubleClick(Token*)),
             m_ui->dropTarget, SLOT(insertToken(Token*)) );
    connect( m_ui->dropTarget, SIGNAL(tokenSelected(Token*)),
             SLOT(slotTokenSelected(Token*)) );
    connect( m_ui->dropTarget, SIGNAL(changed()),
             SLOT(updateSearchEdit()) ); // in case someone dragged a token around.

    connect( m_ui->searchEdit, SIGNAL(textEdited(QString)),
             SLOT(slotSearchEditChanged(QString)) );
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
    m_ui->tpTokenPool->addToken( tokenForField( Meta::valLastPlayed ) );
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

EditFilterDialog::Filter &
EditFilterDialog::filterForToken( Token *token )
{
    // a new token!
    if( !m_filters.contains( token ) ) {
        Filter newFilter;
        newFilter.filter.setField( token->value() );
        newFilter.inverted = false;

        m_filters.insert( token, newFilter );
        connect( token, SIGNAL(destroyed(QObject*)),
                 this, SLOT(slotTokenDestroyed(QObject*)) );
    }

    return m_filters[token];
}

void
EditFilterDialog::slotTokenSelected( Token *token )
{
    DEBUG_BLOCK;

    if( m_curToken == token )
        return; // nothing to do

    m_curToken = token;

    if( m_curToken && m_curToken->value() > Meta::valCustom )   // OR / AND tokens case
        m_curToken = 0;

    updateAttributeEditor();
}

void
EditFilterDialog::slotTokenDestroyed( QObject *token )
{
    DEBUG_BLOCK

    m_filters.take( qobject_cast<Token*>(token) );
    if( m_curToken == token )
    {
        m_curToken = 0;
        updateAttributeEditor();
    }

    updateSearchEdit();
}


void
EditFilterDialog::slotAttributeChanged( const MetaQueryWidget::Filter &newFilter )
{
    DEBUG_BLOCK;

    if( m_curToken )
        m_filters[m_curToken].filter = newFilter;

    updateSearchEdit();
}

void
EditFilterDialog::slotInvert( bool checked )
{
    if( m_curToken )
        m_filters[m_curToken].inverted = checked;

    updateSearchEdit();
}

void
EditFilterDialog::slotSeparatorChange()
{
    if( m_ui->rbAnd->isChecked() )
        m_separator = QString( " AND " );
    else
        m_separator = QString( " OR " );

    updateSearchEdit();
}

void
EditFilterDialog::slotSearchEditChanged( const QString &filterText )
{
    updateDropTarget( filterText );
    updateAttributeEditor();
}

void
EditFilterDialog::slotReset()
{
    m_ui->dropTarget->clear();
    m_ui->rbAnd->setChecked( true );

    updateAttributeEditor();
    updateSearchEdit();
}

void
EditFilterDialog::accept()
{
    emit filterChanged( filter() );
    KDialog::accept();
}

void
EditFilterDialog::updateAttributeEditor()
{
    DEBUG_BLOCK;

    if( m_isUpdating )
        return;
    m_isUpdating = true;

    if( m_curToken )
    {
        Filter &filter = filterForToken( m_curToken );

        m_ui->mqwAttributeEditor->setFilter( filter.filter );
        m_ui->cbInvert->setChecked( filter.inverted );
    }

    m_ui->mqwAttributeEditor->setEnabled( ( bool )m_curToken );
    m_ui->cbInvert->setEnabled( ( bool )m_curToken );

    m_isUpdating = false;
}

void
EditFilterDialog::updateSearchEdit()
{
    DEBUG_BLOCK;

    if( m_isUpdating )
        return;
    m_isUpdating = true;

    m_ui->searchEdit->setText( filter() );

    m_isUpdating = false;
}

void
EditFilterDialog::updateDropTarget( const QString &text )
{
    DEBUG_BLOCK;

    if( m_isUpdating )
        return;
    m_isUpdating = true;

    m_ui->dropTarget->clear();

    // some code duplications, see Collections::semanticDateTimeParser

    ParsedExpression parsed = ExpressionParser::parse( text );
    bool AND = false; // need an AND token
    bool OR = false; // need an OR token
    bool isDateAbsolute = false;
    foreach( const or_list &orList, parsed )
    {
        foreach( const expression_element &elem, orList )
        {
            if( AND )
                m_ui->dropTarget->insertToken( AND_TOKEN_CONSTRUCT );
            else if( OR )
                m_ui->dropTarget->insertToken( OR_TOKEN_CONSTRUCT );

            Filter filter;
            filter.filter.setField( !elem.field.isEmpty() ? Meta::fieldForName( elem.field ) : 0 );
            if( filter.filter.field() == Meta::valRating )
            {
                filter.filter.numValue = 2 * elem.text.toFloat();
            }
            else if( filter.filter.isDate() )
            {
                QString strTime = elem.text;

                // parse date using local settings
                KLocalizedDate localizedDate = KLocalizedDate::readDate( strTime, KLocale::ShortFormat );

                // parse date using a backup standard independent from local settings
                QRegExp shortDateReg("(\\d{1,2})[-.](\\d{1,2})");
                QRegExp longDateReg("(\\d{1,2})[-.](\\d{1,2})[-.](\\d{4})");
                // NOTE for absolute time specifications numValue is a unix timestamp,
                // for relative time specifications numValue is a time difference in seconds 'pointing to the past'
                if( localizedDate.isValid() )
                {
                    filter.filter.numValue = QDateTime( localizedDate.date() ).toTime_t();
                    isDateAbsolute = true;
                }
                else if( strTime.contains(shortDateReg) )
                {
                    filter.filter.numValue = QDateTime( QDate( QDate::currentDate().year(), shortDateReg.cap(2).toInt(), shortDateReg.cap(1).toInt() ) ).toTime_t();
                    isDateAbsolute = true;
                }
                else if( strTime.contains(longDateReg) )
                {
                    filter.filter.numValue = QDateTime( QDate( longDateReg.cap(3).toInt(), longDateReg.cap(2).toInt(), longDateReg.cap(1).toInt() ) ).toTime_t();
                    isDateAbsolute = true;
                }
                else
                {
                    // parse a "#m#d" (discoverability == 0, but without a GUI, how to do it?)
                    int years = 0, months = 0, days = 0, secs = 0;
                    QString tmp;
                    for( int i = 0; i < strTime.length(); i++ )
                    {
                        QChar c = strTime.at( i );
                        if( c.isNumber() )
                        {
                            tmp += c;
                        }
                        else if( c == 'y' )
                        {
                            years += tmp.toInt();
                            tmp.clear();
                        }
                        else if( c == 'm' )
                        {
                            months += tmp.toInt();
                            tmp.clear();
                        }
                        else if( c == 'w' )
                        {
                            days += tmp.toInt() * 7;
                            tmp.clear();
                        }
                        else if( c == 'd' )
                        {
                            days += tmp.toInt();
                            tmp.clear();
                        }
                        else if( c == 'h' )
                        {
                            secs += tmp.toInt() * 60 * 60;
                            tmp.clear();
                        }
                        else if( c == 'M' )
                        {
                            secs += tmp.toInt() * 60;
                            tmp.clear();
                        }
                        else if( c == 's' )
                        {
                            secs += tmp.toInt();
                            tmp.clear();
                        }
                    }
                    filter.filter.numValue = years*365*24*60*60 + months*30*24*60*60 + days*24*60*60 + secs;
                    isDateAbsolute = false;
                }
            }
            else if( filter.filter.isNumeric() )
            {
                filter.filter.numValue = elem.text.toInt();
            }

            if( filter.filter.isDate() )
            {
                switch( elem.match )
                {
                    case expression_element::Less:
                        if( isDateAbsolute )
                            filter.filter.condition = MetaQueryWidget::LessThan;
                        else
                            filter.filter.condition = MetaQueryWidget::NewerThan;
                        break;
                    case expression_element::More:
                        if( isDateAbsolute )
                            filter.filter.condition = MetaQueryWidget::GreaterThan;
                        else
                            filter.filter.condition = MetaQueryWidget::OlderThan;
                        break;
                    default:
                        filter.filter.condition = MetaQueryWidget::Equals;
                }
            }
            else if( filter.filter.isNumeric() )
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
                    case expression_element::Contains:
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
                    case expression_element::Less:
                    case expression_element::More:
                        break;
                }
                filter.filter.value = elem.text;
            }

            filter.inverted = elem.negate;

            Token *nToken = filter.filter.field()
                            ? tokenForField( filter.filter.field() )
                            : SIMPLE_TEXT_CONSTRUCT;
            m_filters.insert( nToken, filter );
            connect( nToken, SIGNAL(destroyed(QObject*)),
                     this, SLOT(slotTokenDestroyed(QObject*)) );

            m_ui->dropTarget->insertToken( nToken );

            OR = true;
        }
        OR = false;
        AND = true;
    }

    m_isUpdating = false;
}


QString
EditFilterDialog::filter()
{
    QString filterString;

    QList < Token *> tokens = m_ui->dropTarget->tokensAtRow();
    bool join = false;
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
        else
        {
            if( join )
                filterString.append( m_separator );
            Filter &filter = filterForToken( token );
            filterString.append( filter.filter.toString( filter.inverted ) );
            join = true;
        }
    }

    return filterString;
}


