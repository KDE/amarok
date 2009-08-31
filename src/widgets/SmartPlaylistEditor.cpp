/****************************************************************************************
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2005 Isaiah Damron <xepo@trifault.net>                                 *
 * Copyright (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>                             *
 * Copyright (c) 2007 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#define DEBUG_PREFIX "SmartPlaylistEditor"

#include "SmartPlaylistEditor.h"

#include "Debug.h"
#include "querybuilder.h"
#include "metabundle.h"

#include <KComboBox>
#include <KLineEdit>
#include <KLocale>
#include <kmountpoint.h>
#include <KVBox>

#include <QCheckBox>
#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QStringList>
#include <QToolButton>


enum Fields
{
    FArtist = 0,
    FComposer,
    FAlbum,
    FGenre,
    FTitle,
    FLength,
    FTrack,
    FYear,
    FComment,
    FPlayCounter,
    FScore,
    FRating,
    FFirstPlay,
    FLastPlay,
    FModfiedDate,
    FFilePath,
    FBPM,
    FMountPoint,
    FBitRate,
    FLabel
};


QStringList m_fields;
QStringList m_dbFields;
QStringList m_expandableFields;
QStringList m_expandableDbFields;



SmartPlaylistEditor::SmartPlaylistEditor( QString defaultName, QWidget *parent, const char *name )
    : KDialog( parent )
{
    setObjectName( name );
    setCaption( i18n("Create Smart Playlist") );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    init(defaultName);
    addCriteriaAny();
    addCriteriaAll();
}


SmartPlaylistEditor::SmartPlaylistEditor( QWidget *parent, QDomElement xml, const char *name)
    : KDialog( parent )
{
    setObjectName( name );
    setCaption( i18n("Edit Smart Playlist") );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );


    init( xml.attribute( "name" ) );
    // matches
    QDomNodeList matchesList =  xml.elementsByTagName( "matches" );
    bool matchedANY = false, matchedALL = false;

    m_matchAllCheck->setChecked( true );
    m_matchAnyCheck->setChecked( true );

    for (int i = 0, m = matchesList.count(); i<m; i++) {
        QDomElement matches = matchesList.item(i).toElement();
        QDomNodeList criteriaList =  matches.elementsByTagName( "criteria" );

        if ( criteriaList.count() ) {
            for (int j = 0, c=criteriaList.count() ; j<c; ++j ) {
                QDomElement criteria = criteriaList.item(j).toElement();

                if (matches.attribute( "glue" ) == "OR") {
                    matchedANY = true;
                    addCriteriaAny( criteria );
                }
                else {
                    matchedALL = true;
                    addCriteriaAll( criteria );
                }
            }
        }
    }

    if ( !matchedALL ) {
        addCriteriaAll();
        m_matchAllCheck->setChecked( false );
    }

    if ( !matchedANY ) {
        m_matchAnyCheck->setChecked( false );
        addCriteriaAny( );
    }

    // orderby
    QDomNodeList orderbyList =  xml.elementsByTagName( "orderby" );
    if ( orderbyList.count() ) {
        m_orderCheck->setChecked( true );
        QDomElement orderby = orderbyList.item(0).toElement(); // we only allow one orderby node

        //random is always the last one.
        int dbfield = orderby.attribute( "field" ) == "random" ? m_dbFields.count() : m_dbFields.indexOf( orderby.attribute( "field" ) );

        m_orderCombo->setCurrentIndex( dbfield );
        updateOrderTypes( dbfield );
        if ( orderby.attribute( "order" ) == "DESC" || orderby.attribute( "order" ) == "weighted" )
            m_orderTypeCombo->setCurrentIndex( 1 );
        else if ( orderby.attribute( "order" ) == "ratingweighted" )
            m_orderTypeCombo->setCurrentIndex( 2 );
        else
            m_orderTypeCombo->setCurrentIndex( 0 );
    }
    // limit
    if  ( xml.hasAttribute( "maxresults" ) ) {
        m_limitCheck->setChecked( true );
        m_limitSpin->setValue( xml.attribute( "maxresults" ).toInt() );
    }

    // expand by
    QDomNodeList expandbyList =  xml.elementsByTagName( "expandby" );
    if ( expandbyList.count() ) {
        m_expandCheck->setChecked( true );
        QDomElement expandby = expandbyList.item(0).toElement(); // we only allow one orderby node

        int dbfield = m_expandableFields.indexOf( expandby.attribute( "field" ) );
        m_expandCombo->setCurrentIndex( dbfield );
    }
}


void SmartPlaylistEditor::init(QString defaultName)
{
    KVBox *vbox = new KVBox( this );
    setMainWidget( vbox );


    m_fields.clear();
    m_fields << i18n("Artist") << i18n("Composer") << i18n("Album") << i18n("Genre") << i18n("Title") << i18n("Length")
             << i18n("Track #") << i18n("Year") << i18n("Comment") << i18n("Play Counter")
             << i18n("Score") << i18n( "Rating" ) << i18n("First Play")
             << i18n("Last Play") << i18n("Modified Date") << i18n("File Path")
             << i18n("BPM") << i18n("Mount Point") << i18n( "Bitrate" ) << i18n( "Label" );

    m_dbFields.clear();
    m_dbFields << "artist.name" << "composer.name" << "album.name" << "genre.name" << "tags.title" << "tags.length"
               << "tags.track" << "year.name" << "tags.comment" << "statistics.playcounter"
               << "statistics.percentage" << "statistics.rating" << "statistics.createdate"
               << "statistics.accessdate" << "tags.createdate" << "tags.url"
               << "tags.bpm" << "devices.lastmountpoint" << "tags.bitrate" << "labels.name";

    m_expandableFields.clear();
    m_expandableFields << i18n("Artist") << i18n("Composer") << i18n("Album") << i18n("Genre") <<  i18n("Year") << i18n( "Label" );

    m_expandableDbFields.clear();
    m_expandableDbFields << "artist.name" << "composer.name" << "album.name" << "genre.name" << "year.name" << "labels.name";

    KHBox *hbox = new KHBox( mainWidget() );
    hbox->setSpacing( 5 );
    new QLabel( i18n("Playlist name:"), hbox );
    m_nameLineEdit = new KLineEdit( defaultName, hbox );

    QFrame *sep = new QFrame( mainWidget() );
    sep->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    //match box (any)
    KHBox *matchAnyBox = new KHBox( mainWidget() );
    m_matchAnyCheck = new QCheckBox( i18n("Match Any of the following conditions" ), matchAnyBox );
    matchAnyBox->setStretchFactor( new QWidget( matchAnyBox ), 1 );

    //criteria box
    m_criteriaAnyGroupBox = new QGroupBox( QString(), mainWidget() );
    QVBoxLayout *anyBoxLayout = new QVBoxLayout();
    m_criteriaAnyGroupBox->setLayout( anyBoxLayout );

    //match box (all)
    KHBox *matchAllBox = new KHBox( mainWidget() );
    m_matchAllCheck = new QCheckBox( i18n("Match All of the following conditions" ), matchAllBox );
    matchAllBox->setStretchFactor( new QWidget( matchAllBox ), 1 );

    //criteria box
    m_criteriaAllGroupBox = new QGroupBox( QString(), mainWidget() );
    QVBoxLayout *allBoxLayout = new QVBoxLayout();
    m_criteriaAllGroupBox->setLayout( allBoxLayout );

    //order box
    KHBox *hbox2 = new KHBox( mainWidget() );
    m_orderCheck = new QCheckBox( i18n("Order by"), hbox2 );
    KHBox *orderBox = new KHBox( hbox2 );
    orderBox->setSpacing( 5 );
    //fields combo
    m_orderCombo = new KComboBox( orderBox );
    m_orderCombo->insertItems( 0, m_fields );
    m_orderCombo->addItem( i18n("Random") );
    //order type
    m_orderTypeCombo = new KComboBox( orderBox );
    updateOrderTypes(0); // populate the new m_orderTypeCombo
    hbox2->setStretchFactor( new QWidget( hbox2 ), 1 );

    //limit box
    KHBox *hbox1 = new KHBox( mainWidget() );
    m_limitCheck = new QCheckBox( i18n("Limit to"), hbox1 );
    KHBox *limitBox = new KHBox( hbox1 );
    limitBox->setSpacing( 5 );
    m_limitSpin = new QSpinBox( limitBox );
    m_limitSpin->setMinimum( 1 );
    m_limitSpin->setMaximum( 100000 );
    m_limitSpin->setValue( 15 );
    new QLabel( i18n("tracks"), limitBox );
    hbox1->setStretchFactor( new QWidget( hbox1 ), 1 );

    //Expand By
    KHBox *hbox3 = new KHBox( mainWidget() );
    m_expandCheck = new QCheckBox( i18n("Expand by"), hbox3 );
    KHBox *expandBox = new KHBox( hbox3 );
    expandBox->setSpacing( 5 );
    m_expandCombo = new KComboBox( expandBox );
    m_expandCombo->insertItems( 0, m_expandableFields );
    hbox3->setStretchFactor( new QWidget( hbox3 ), 1 );

    //add stretch
    static_cast<KHBox *>(mainWidget())->setStretchFactor(new QWidget(mainWidget()), 1);

    connect( m_matchAnyCheck, SIGNAL( toggled(bool) ), m_criteriaAnyGroupBox, SLOT( setEnabled(bool) ) );
    connect( m_matchAllCheck, SIGNAL( toggled(bool) ), m_criteriaAllGroupBox, SLOT( setEnabled(bool) ) );
    connect( m_orderCheck, SIGNAL( toggled(bool) ), orderBox, SLOT( setEnabled(bool) ) );
    connect( m_limitCheck, SIGNAL( toggled(bool) ), limitBox, SLOT(  setEnabled(bool) ) );
    connect( m_expandCheck, SIGNAL( toggled(bool) ), expandBox, SLOT( setEnabled(bool) ) );
    connect( m_orderCombo, SIGNAL( activated(int) ), this, SLOT( updateOrderTypes(int) ) );

    m_criteriaAnyGroupBox->setEnabled( false );
    m_criteriaAllGroupBox->setEnabled( false );

    orderBox->setEnabled( false );
    limitBox->setEnabled( false );
    expandBox->setEnabled( false );

    m_nameLineEdit->setFocus();

    resize( 550, 200 );
}


void SmartPlaylistEditor::addCriteriaAny()
{
    CriteriaEditor *criteria= new CriteriaEditor( this, m_criteriaAnyGroupBox, criteriaAny );
    m_criteriaAnyGroupBox->layout()->addWidget( criteria );
    m_criteriaEditorAnyList.append( criteria );
    m_criteriaEditorAnyList.first()->enableRemove( m_criteriaEditorAnyList.count() > 1 );
}

void SmartPlaylistEditor::addCriteriaAll()
{
    CriteriaEditor *criteria= new CriteriaEditor( this, m_criteriaAllGroupBox, criteriaAll );
    m_criteriaAllGroupBox->layout()->addWidget( criteria );
    m_criteriaEditorAllList.append( criteria );
    m_criteriaEditorAllList.first()->enableRemove( m_criteriaEditorAllList.count() > 1 );
}

void SmartPlaylistEditor::addCriteriaAny( QDomElement &xml )
{
    CriteriaEditor *criteria = new CriteriaEditor( this, m_criteriaAnyGroupBox, criteriaAny, xml );
    m_criteriaAnyGroupBox->layout()->addWidget( criteria );
    m_criteriaEditorAnyList.append( criteria );
    m_criteriaEditorAnyList.first()->enableRemove( m_criteriaEditorAnyList.count() > 1 );
}

void SmartPlaylistEditor::addCriteriaAll( QDomElement &xml )
{
    CriteriaEditor *criteria = new CriteriaEditor( this, m_criteriaAllGroupBox, criteriaAll, xml );
    m_criteriaAllGroupBox->layout()->addWidget( criteria );
    m_criteriaEditorAllList.append( criteria );
    m_criteriaEditorAllList.first()->enableRemove( m_criteriaEditorAllList.count() > 1 );
}

void SmartPlaylistEditor::removeCriteriaAny( CriteriaEditor *criteria )
{
    m_criteriaEditorAnyList.removeAll( criteria );
    criteria->deleteLater();
    resize( size().width(), sizeHint().height() );

    if( m_criteriaEditorAnyList.count() == 1 )
	m_criteriaEditorAnyList.first()->enableRemove( false );
}

void SmartPlaylistEditor::removeCriteriaAll( CriteriaEditor *criteria )
{
    m_criteriaEditorAllList.removeAll( criteria );
    criteria->deleteLater();
    resize( size().width(), sizeHint().height() );

    if( m_criteriaEditorAllList.count() == 1 )
	m_criteriaEditorAllList.first()->enableRemove( false );
}

void SmartPlaylistEditor::updateOrderTypes( int index )
{
    int currentOrderType = m_orderTypeCombo->currentIndex();
    if( index == m_orderCombo->count()-1 ) {  // random order selected
        m_orderTypeCombo->clear();
        m_orderTypeCombo->addItem( i18n("Completely Random") );
        m_orderTypeCombo->addItem( i18n("Score Weighted") );
        m_orderTypeCombo->addItem( i18n("Rating Weighted") );
    }
    else {  // ordinary order column selected
        m_orderTypeCombo->clear();
        m_orderTypeCombo->addItem( i18n("Ascending") );
        m_orderTypeCombo->addItem( i18n("Descending") );
    }
    if( currentOrderType < m_orderTypeCombo->count() )
        m_orderTypeCombo->setCurrentIndex( currentOrderType );
    m_orderTypeCombo->setFont(m_orderTypeCombo->font());  // invalidate size hint
    m_orderTypeCombo->updateGeometry();
}

QDomElement SmartPlaylistEditor::result()
{
    QDomDocument doc;
    QDomNode node = doc.namedItem( "smartplaylists" );
    QDomElement nodeE;
    nodeE = node.toElement();

    QDomElement smartplaylist = doc.createElement( "smartplaylist" );

    smartplaylist.setAttribute( "name", name() );

    // Limit
    if ( m_limitCheck->isChecked() )
        smartplaylist.setAttribute( "maxresults", m_limitSpin->value() );

    nodeE.appendChild( smartplaylist );
    // Matches
    if( m_matchAnyCheck->isChecked() )
    {
        QDomElement matches = doc.createElement("matches");
        smartplaylist.appendChild( matches );
        // Iterate through all criteria list
        foreach( CriteriaEditor *criteriaeditor, m_criteriaEditorAnyList )
            matches.appendChild( doc.importNode( criteriaeditor->getDomSearchCriteria( doc ), true ) );

        matches.setAttribute( "glue",  "OR" );
        smartplaylist.appendChild( matches );
    }

    if( m_matchAllCheck->isChecked() )
    {
        QDomElement matches = doc.createElement("matches");
        smartplaylist.appendChild( matches );
        // Iterate through all criteria list
        foreach( CriteriaEditor *criteriaeditor, m_criteriaEditorAllList )
            matches.appendChild( doc.importNode( criteriaeditor->getDomSearchCriteria( doc ), true ) );

        matches.setAttribute( "glue",  "AND" );
        smartplaylist.appendChild( matches );
    }

    // Order By
    if( m_orderCheck->isChecked() ) {
        QDomElement orderby = doc.createElement("orderby");
        if (m_orderCombo->currentIndex() != m_orderCombo->count()-1) {
            orderby.setAttribute( "field", m_dbFields[ m_orderCombo->currentIndex() ] );
            orderby.setAttribute( "order", m_orderTypeCombo->currentIndex() == 1 ? "DESC" : "ASC" );
        } else {
            orderby.setAttribute( "field", "random" );
            QString order;
            if ( m_orderTypeCombo->currentIndex() == 0 )
                order = "random";
            else if ( m_orderTypeCombo->currentIndex() == 1 )
                order = "weighted";
            else
                order = "ratingweighted";
            orderby.setAttribute( "order", order );
        }

        smartplaylist.appendChild( orderby );
    }

    if( m_expandCheck->isChecked() ) {
        QDomElement expandBy = doc.createElement("expandby");
        expandBy.setAttribute( "field", m_expandableFields[ m_expandCombo->currentIndex() ] );
        smartplaylist.appendChild( expandBy );
    }

    return (smartplaylist);
}

/////////////////////////////////////////////////////////////////////////////
//    CLASS CriteriaEditor
////////////////////////////////////////////////////////////////////////////

CriteriaEditor::CriteriaEditor( SmartPlaylistEditor *editor, QWidget *parent, int criteriaType, QDomElement criteria )
    : KHBox( parent )
    , m_playlistEditor( editor )
    , m_currentValueType( -1 )
{
    setSpacing( 5 );

    m_fieldCombo = new KComboBox( this );
    m_fieldCombo->insertItems( 0, m_fields );

    m_criteriaCombo = new KComboBox( this );

    m_editBox = new KHBox( this );
    m_editBox->setSpacing( 5 );
    setStretchFactor( m_editBox, 1 );

    m_addButton = new QToolButton( this );
    m_addButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
    m_addButton->setText( "+" );
    m_removeButton = new QToolButton( this );
    m_removeButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
    m_removeButton->setText( "-" );

    connect( m_fieldCombo,    SIGNAL( activated(int) ), SLOT( slotFieldSelected(int) ) );
    connect( m_criteriaCombo, SIGNAL( activated(int) ), SLOT( loadEditWidgets() ) );
    if (criteriaType == SmartPlaylistEditor::criteriaAny) {
	connect( m_addButton, SIGNAL( clicked() ), editor, SLOT( addCriteriaAny() ) );
	connect( m_removeButton, SIGNAL( clicked() ), SLOT( slotRemoveCriteriaAny() ) );
    }
    else {
	connect( m_addButton, SIGNAL( clicked() ), editor, SLOT( addCriteriaAll() ) );
	connect( m_removeButton, SIGNAL( clicked() ), SLOT( slotRemoveCriteriaAll() ) );
    }

    if ( !criteria.isNull() ) {
        int field = m_dbFields.indexOf( criteria.attribute( "field" ) );
        QString condition = criteria.attribute("condition");


        QStringList values; //List of the values (only one item, unless condition is "is between")
        QDomNodeList domvalueList = criteria.elementsByTagName( "value" );
        for (int j = 0, c=domvalueList.count() ; j<c; ++j ) {
                values << domvalueList.item(j).toElement().text();
        }

        //Set the selected field

        m_fieldCombo->setCurrentIndex( field );
        slotFieldSelected( field );
        int valueType = getValueType( field );
        //Load the right set of criterias for this type, in the dialog
        loadCriteriaList( valueType, condition );

        loadEditWidgets();

        switch( valueType ) {
            case String: //fall through
            case AutoCompletionString:
            {
                m_lineEdit->setText( values.first() );
                break;
            }
            case Year:    //fall through
            case Number:
            {
                m_intSpinBox1->setValue( values.first().toInt() );
                if( condition == i18n("is between") )
                    m_intSpinBox2->setValue( values.last().toInt() );
                break;
            }
            case Rating:
            {
                m_comboBox->setCurrentIndex( ratingToIndex( values.first().toInt() ) );
                if( condition == i18n("is between") )
                    m_comboBox2->setCurrentIndex( ratingToIndex( values.last().toInt() ) );
                break;
            }
            case Date:
            {
                if( condition == i18n("is in the last") || condition == i18n("is not in the last") ) {
                    m_intSpinBox1->setValue( values.first().toInt() );
                    QString period = criteria.attribute("period");
                    if (period=="days" || period.isEmpty() )
                        m_dateCombo->setCurrentIndex(0);
                    else if (period=="months")
                        m_dateCombo->setCurrentIndex(1);
                    else
                        m_dateCombo->setCurrentIndex(2);
                }
                else {
                    QDateTime dt;
                    dt.setTime_t( values.first().toUInt() );
                    m_dateEdit1->setDate( dt.date() );
                    if( condition == i18n("is between") ) {
                        dt.setTime_t( values.last().toUInt() );
                        m_dateEdit2->setDate( dt.date() );
                    }
                }
                break;
            }
            case Length:
            {
                m_intSpinBox1->setValue( values.first().toInt() );
                if( condition == i18n("is between") )
                    m_intSpinBox2->setValue( values.last().toInt() );
                QString period = criteria.attribute( "period" );
                if ( period == "seconds" || period.isEmpty() ) //for compatibility
                    m_lengthCombo->setCurrentIndex( 0 );
                else if ( period == "minutes" )
                    m_lengthCombo->setCurrentIndex( 1 );
                else
                    m_lengthCombo->setCurrentIndex( 2 );
                break;
            }
            default: ;
        };
    }
    else
        slotFieldSelected( 0 );
    show();
}


CriteriaEditor::~CriteriaEditor()
{
}

QDomElement CriteriaEditor::getDomSearchCriteria( QDomDocument &doc )
{
    QDomElement criteria = doc.createElement( "criteria" );
    QString field = m_dbFields[ m_fieldCombo->currentIndex() ];
    QString condition = m_criteriaCombo->currentText();

    criteria.setAttribute( "condition", condition );
    criteria.setAttribute( "field", field );

    QStringList values;
    // Get the proper value(s)
    switch( getValueType( m_fieldCombo->currentIndex() ) ) {
         case String: // fall through
         case AutoCompletionString:
            values << m_lineEdit->text();
            break;
         case Year: // fall through
         case Number:
         {
            values << QString::number( m_intSpinBox1->value() );
            if( condition == i18n("is between")  )
                values << QString::number( m_intSpinBox2->value() );
            break;
         }
         case Rating:
         {
            values << QString::number( indexToRating( m_comboBox->currentIndex() ) );
            if( condition == i18n("is between")  )
                    values << QString::number( indexToRating( m_comboBox2->currentIndex() ) );
            break;
         }
         case Date:
         {
            if( condition == i18n("is in the last") || condition == i18n("is not in the last") ) {
                values << QString::number( m_intSpinBox1->value() );
                // 0 = days; 1=months; 2=years
                criteria.setAttribute( "period", !m_dateCombo->currentIndex() ? "days" : (m_dateCombo->currentIndex() == 1 ? "months" : "years") );
            }
            else {
                values << QString::number( QDateTime( m_dateEdit1->date() ).toTime_t() );
                if( condition == i18n("is between")  ) {
                    values << QString::number( QDateTime( m_dateEdit2->date() ).toTime_t() );
               }
            }
            break;
         }
         case Length:
         {
            values << QString::number( m_intSpinBox1->value() );
            // 0 = seconds, 1=minutes, 2=hours
            criteria.setAttribute( "period", !m_lengthCombo->currentIndex() ? "seconds" : (m_lengthCombo->currentIndex() == 1 ? "minutes" : "hours") );
            if( condition == i18n( "is between" ) ) {
                values << QString::number( m_intSpinBox2->value() );
            }
            break;
         }
         default: ;
    }
    oldForeach( values ) {
        QDomElement value = doc.createElement( "value" );
        QDomText t = doc.createTextNode( *it );
        value.appendChild( t );
        criteria.appendChild( value );
    }
    return (criteria);
}


QString CriteriaEditor::getSearchCriteria()
{
    QString searchCriteria;
    QString field = m_dbFields[ m_fieldCombo->currentIndex() ];
    QString criteria = m_criteriaCombo->currentText();

    if( field.isEmpty() )
        return QString();

    if ( ( field=="statistics.playcounter" || field=="statistics.rating" || field=="statistics.percentage" || field=="statistics.accessdate" || field=="statistics.createdate") )
        searchCriteria += "COALESCE(" + field + ",0)";
    else
        searchCriteria += field;

    QString value;
    switch( getValueType( m_fieldCombo->currentIndex() ) ) {
        case String:
        case AutoCompletionString:
            value = m_lineEdit->text();
            break;
        case Year:    //fall through
        case Number:
            value = QString::number( m_intSpinBox1->value() );
            if( criteria == i18n("is between")  )
                value += " AND " + QString::number( m_intSpinBox2->value() );
            break;
        case Rating:
        {
            value = QString::number( indexToRating( m_comboBox->currentIndex() ) );
            if( criteria == i18n("is between")  )
                value += " AND " + QString::number( indexToRating( m_comboBox2->currentIndex() ) );
            break;
        }
        case Date:
        {
            if( criteria == i18n("is in the last") || criteria == i18n("is not in the last") ) {
                int n = m_intSpinBox1->value();
                int time;
                if( m_dateCombo->currentIndex() == 0 ) //days
                    time=86400*n;
                else if( m_dateCombo->currentIndex() == 1 ) //months
                    time=86400*30*n;
                else time=86400*365*n; //years
                value += "(*CurrentTimeT*)" + QString(" - %1 AND ").arg(time) + "(*CurrentTimeT*)";
            }
            else {
                QDateTime datetime1( m_dateEdit1->date() );
                value += QString::number( datetime1.toTime_t() );
                if( criteria == i18n("is between")  ) {
                    QDateTime datetime2( m_dateEdit2->date() );
                    value += " AND " + QString::number( datetime2.toTime_t() );
                }
                else
                    value += " AND " + QString::number( datetime1.addDays( 1 ).toTime_t() );
            }
            break;
        }
        case Length:
        {
            int n = m_intSpinBox1->value();
            int time;
            if( m_lengthCombo->currentIndex() == 0 ) //seconds
                time = n;
            else if( m_lengthCombo->currentIndex() == 1 ) //minutes
                time = 60*n;
            else
                time = 3600*n; //hours
            value = QString::number( time );
            if( criteria == i18n("is between")  ) {
                int n2 = m_intSpinBox2->value();
                int time2;
                if( m_lengthCombo->currentIndex() == 0 ) //seconds
                    time2 = n2;
                else if( m_lengthCombo->currentIndex() == 1 ) //minutes
                    time2 = 60*n2;
                else
                    time2 = 3600*n2; //hours
                value += " AND " + QString::number( time2 );
            }
            break;
        }
        default: ;
    };


    if( criteria == i18n("contains") )
        searchCriteria += CollectionDB::likeCondition( value, true, true );
    else if( criteria == i18n("does not contain") )
        searchCriteria += " NOT " + CollectionDB::likeCondition( value, true, true );
    else if( criteria == i18n("is") ) {
        if( m_currentValueType == Date )
            searchCriteria += " BETWEEN ";
        else
            searchCriteria += " = ";
        if( m_currentValueType == String || m_currentValueType == AutoCompletionString )
            value.prepend("'").append("'");
        searchCriteria += value;
    }
    else if( criteria == i18n("is not") ) {
        if( m_currentValueType == Date )
            searchCriteria += " NOT BETWEEN ";
        else
            searchCriteria += " <> ";
        if( m_currentValueType == String || m_currentValueType == AutoCompletionString )
            value.prepend("'").append("'");
        searchCriteria += value;
    }
    else if( criteria == i18n("starts with") )
    {
        if( field == "tags.url" )
        {
            if( value.startsWith( '/' ) )
                value = '.' + value;
            if( !value.startsWith( "./" ) )
                value = "./" + value;
        }
        searchCriteria += CollectionDB::likeCondition( value, false, true );
    }
    else if( criteria == i18n("does not start with") )
    {
        if( field == "tags.url" )
        {
            if( value.startsWith( '/' ) )
                value = '.' + value;
            if( !value.startsWith( "./" ) )
                value = "./" + value;
        }
        searchCriteria += " NOT " + CollectionDB::likeCondition( value, false, true );
    }
    else if( criteria == i18n("ends with") )
        searchCriteria += CollectionDB::likeCondition( value, true, false );
    else if( criteria == i18n("does not end with") )
        searchCriteria += " NOT " + CollectionDB::likeCondition( value, true, false );
    else if( criteria == i18n("is greater than") || criteria == i18n("is after") )
        searchCriteria += " > " + value;
    else if( criteria == i18n("is smaller than") || criteria == i18n("is before" ) )
        searchCriteria += " < " + value;
    else if( criteria == i18n("is between") || criteria == i18n("is in the last") )
        searchCriteria += " BETWEEN " + value;
    else if( criteria == i18n("is not in the last") )
        searchCriteria += " NOT BETWEEN " + value;

    return searchCriteria;
}


void CriteriaEditor::setSearchCriteria( const QString & )
{
    //TODO
}


void CriteriaEditor::enableRemove( bool enable )
{
    m_removeButton->setEnabled( enable );
}


void CriteriaEditor::slotRemoveCriteriaAny()
{
    m_playlistEditor->removeCriteriaAny( this );
}

void CriteriaEditor::slotRemoveCriteriaAll()
{
    m_playlistEditor->removeCriteriaAll( this );
}

void CriteriaEditor::slotAddCriteriaAny()
{
    m_playlistEditor->addCriteriaAny();
}

void CriteriaEditor::slotAddCriteriaAll()
{
    m_playlistEditor->addCriteriaAll();
}

void CriteriaEditor::slotFieldSelected( int field )
{
    int valueType = getValueType( field );
    loadCriteriaList( valueType );
    loadEditWidgets();
    m_currentValueType = valueType;

    //enable auto-completion for artist, album, composer, label, mountpoint and genre
    if( valueType == AutoCompletionString ) {
        QStringList items;
        m_comboBox->clear();
        m_comboBox->completionObject()->clear();

        int currentField = m_fieldCombo->currentIndex();
        if( currentField == FArtist ) //artist
           items = CollectionDB::instance()->artistList();
        else if( currentField == FComposer ) //composer
           items = CollectionDB::instance()->composerList();
        else if( currentField == FAlbum ) //album
           items = CollectionDB::instance()->albumList();
        else if (currentField == FLabel ) //label
            items = CollectionDB::instance()->labelList();
        else if (currentField == FMountPoint ) //mount point
        {
            KMountPoint::List mountpoints = KMountPoint::currentMountPoints( KMountPoint::NeedRealDeviceName );
            oldForeachType( KMountPoint::List, mountpoints )
            {
                /* This code is adapted from KDE mediamanager's fstabbackend.cpp
                 * Copyright Kvin Ottens, Bernhard Rosenkraenzer, and from looking
                 * at the commit messages a few other guys who didn't add their name to the header.
                 */
                QString device = (*it)->realDeviceName();
                QString fs = (*it)->mountType();
                QString mountpoint = (*it)->mountPoint();
                if ( fs != "swap"
                     && fs != "tmpfs"
                     && fs != "sysfs"
                     && fs != "fdescfs"
                     && fs != "kernfs"
                     && fs != "usbfs"
                     && !fs.contains( "proc" )
                     && fs != "unknown"
                     && fs != "none"
                     && fs != "sunrpc"
                     && fs != "none"
                     && device != "tmpfs"
                     && device.indexOf("shm") == -1
                     && mountpoint != "/dev/swap"
                     && mountpoint != "/dev/pts"
                     && mountpoint.indexOf("/proc") != 0
                     && mountpoint.indexOf("/sys") != 0
                     || fs.indexOf( "smb" ) != -1
                     || fs.indexOf( "cifs" ) != -1
                     || fs.indexOf( "nfs" ) != -1
                )
                    items << mountpoint;
            }
        }
        else  //genre
           items = CollectionDB::instance()->genreList();

        m_comboBox->insertItems( 0, items );
        m_comboBox->completionObject()->insertItems( items );
        m_comboBox->completionObject()->setIgnoreCase( true );
        m_comboBox->setItemText( m_comboBox->currentIndex(), "" );
        m_comboBox->setFocus();
    }
}


void CriteriaEditor::loadEditWidgets()
{
    int valueType = getValueType( m_fieldCombo->currentIndex() );

    if( m_currentValueType == valueType && !(
        m_criteriaCombo->currentText() == i18n( "is between" ) ||
        m_criteriaCombo->currentText() == i18n( "is in the last" ) ||
        m_criteriaCombo->currentText() == i18n( "is not in the last" ) ||
        m_lastCriteria == i18n( "is between" ) ||
        m_lastCriteria == i18n( "is in the last" ) ||
        m_lastCriteria == i18n( "is not in the last" ) ) )
        return;

    /* Store lastCriteria. This information is used above to decide whether it's necessary to change the Widgets */
    m_lastCriteria = m_criteriaCombo->currentText();

    QList<QWidget *> list = qFindChildren<QWidget *>( m_editBox );
    foreach( QWidget *w, list )
        w->deleteLater();

    switch( valueType ) {

        case String:
        {
            m_lineEdit = new KLineEdit( m_editBox );
            m_lineEdit->setFocus();
            m_lineEdit->show();
            break;
        }

        case AutoCompletionString:    //artist, composer, album, genre, label
        {
            m_comboBox = new KComboBox( true, m_editBox );
            m_lineEdit = static_cast<KLineEdit*>( m_comboBox->lineEdit() );
            m_lineEdit->setFocus();
            m_comboBox->setMinimumSize( QSize( 240, 20 ) );
            m_comboBox->show();
            break;
        }

        case Year:    //fall through
        case Number:
        {
            bool yearField = m_fieldCombo->currentText() == i18n("Year");

            m_intSpinBox1 = new QSpinBox( m_editBox );
            int maxValue = 1000;
            if( yearField ) {
                maxValue = QDate::currentDate().year();
                m_intSpinBox1->setValue( maxValue );
            }
            m_intSpinBox1->setMaximum( maxValue );
            m_intSpinBox1->setFocus();
            m_intSpinBox1->show();

            if( m_criteriaCombo->currentText() == i18n("is between") ) {
                m_rangeLabel = new QLabel( i18n("and"), m_editBox );
                m_rangeLabel->setAlignment( Qt::AlignHCenter );
                m_rangeLabel->show();
                m_intSpinBox2 = new QSpinBox( m_editBox );
                if( yearField ) {
                    maxValue = QDate::currentDate().year();
                    m_intSpinBox2->setValue( maxValue );
                }
                m_intSpinBox2->setMaximum( maxValue );
                m_intSpinBox2->show();
            }
            break;
        }

        case Rating:
        {
            const QStringList list = MetaBundle::ratingList();
            m_comboBox = new KComboBox( false, m_editBox );
            m_comboBox->insertItems( 0, list );
            m_comboBox->show();

            if( m_criteriaCombo->currentText() == i18n("is between") ) {
                m_rangeLabel = new QLabel( i18n("and"), m_editBox );
                m_rangeLabel->setAlignment( Qt::AlignHCenter );
                m_rangeLabel->show();
                m_comboBox2 = new KComboBox( false, m_editBox );
                m_comboBox2->insertItems( 0, list );
                m_comboBox2->show();
            }
            break;
        }

        case Date:
        {
            if( m_criteriaCombo->currentText() == i18n("is in the last") ||
                m_criteriaCombo->currentText() == i18n("is not in the last") ) {
                m_intSpinBox1 = new QSpinBox( m_editBox );
                m_intSpinBox1->setMinimum( 1 );
                m_intSpinBox1->show();
                m_dateCombo = new KComboBox( m_editBox );
                m_dateCombo->addItem( i18n("Days") );
                m_dateCombo->addItem( i18n("Months") );
                m_dateCombo->addItem( i18n("Years") );
                m_dateCombo->show();
            }
            else {
                m_dateEdit1 = new QDateEdit( QDate::currentDate(), m_editBox);
                m_dateEdit1->setFocus();
                m_dateEdit1->show();
                if( m_criteriaCombo->currentText() == i18n("is between") ) {
                    m_rangeLabel = new QLabel( i18n("and"), m_editBox );
                    m_rangeLabel->setAlignment( Qt::AlignHCenter );
                    m_rangeLabel->show();
                    m_dateEdit2 = new QDateEdit( QDate::currentDate(), m_editBox);
                    m_dateEdit2->show();
                }
            }

            break;
        }

        case Length:
        {
            m_intSpinBox1 = new QSpinBox( m_editBox );
            int maxValue = 1000;
            m_intSpinBox1->setMaximum( maxValue );
            m_intSpinBox1->setFocus();
            m_intSpinBox1->show();
            if( m_criteriaCombo->currentText() == i18n("is between") ) {
                m_rangeLabel = new QLabel( i18n("and"), m_editBox );
                m_rangeLabel->setAlignment( Qt::AlignHCenter );
                m_rangeLabel->show();
                m_intSpinBox2 = new QSpinBox( m_editBox );
                m_intSpinBox2->setMaximum( maxValue );
                m_intSpinBox2->show();
            }
            m_lengthCombo = new KComboBox( m_editBox );
            m_lengthCombo->addItem( i18n( "Seconds" ) );
            m_lengthCombo->addItem( i18n( "Minutes" ) );
            m_lengthCombo->addItem( i18n( "Hours" ) );
            m_lengthCombo->show();
        }

        default: ;
    };

}


void CriteriaEditor::loadCriteriaList( int valueType, QString condition )
{
    if( m_currentValueType == valueType && condition.isNull() )
        return;

    QStringList items;

    switch( valueType ) {
        case String:
        case AutoCompletionString:
            items << i18n( "contains" ) << i18n( "does not contain" ) << i18n( "is" ) << i18n( "is not" )
                  << i18n( "starts with" ) << i18n( "does not start with" )
                  << i18n( "ends with" ) << i18n( "does not end with" );
            break;

        case Rating:
        case Length:
        case Number:
            items << i18n( "is" ) << i18n( "is not" ) << i18n( "is greater than" ) << i18n( "is smaller than" )
                  << i18n( "is between" );
            break;

        case Year: //fall through
        case Date:
            items << i18n( "is" ) << i18n( "is not" ) << i18n( "is before" ) << i18n( "is after" )
                  << i18n( "is in the last" ) << i18n( "is not in the last" ) << i18n( "is between" );
            break;
        default: ;
    };

    m_criteriaCombo->clear();
    m_criteriaCombo->insertItems( 0, items );

    if( !condition.isEmpty() )
    {
        int index = items.indexOf( condition );
        if( index != -1 )
            m_criteriaCombo->setCurrentIndex( index );
    }
}


int CriteriaEditor::getValueType( int index )
{
    int valueType;

    switch( index ) {
        case FArtist:
        case FComposer:
        case FAlbum:
        case FGenre:
        case FLabel:
        case FMountPoint:
            valueType = AutoCompletionString;
            break;
        case FTitle:
        case FComment:
        case FFilePath:
            valueType = String;
            break;
        case FLength:
            valueType = Length;
            break;
        case FTrack:
        case FScore:
        case FPlayCounter:
        case FBPM:
        case FBitRate:
            valueType = Number;
            break;
        case FRating:
            valueType = Rating;
            break;
        case FYear:
            valueType = Year;
            break;
        default: valueType = Date;
    };

    return valueType;
}


#include "SmartPlaylistEditor.moc"

