
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kseparator.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qspinbox.h>
#include <xine.h>
#include "xine-config.h"


XineConfigEntry::XineConfigEntry( QWidget *parent, amaroK::PluginConfig *pluginConfig, int row, xine_cfg_entry_t *entry )
    : m_valueChanged( false )
    , m_numValue( entry->num_value )
    , m_key( entry->key )
    , m_stringValue( entry->str_value )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();

    switch (entry->type)
    {
    case XINE_CONFIG_TYPE_STRING:
    {
        stringEdit = new KLineEdit( m_stringValue, parent );
        grid->addWidget( stringEdit, row, 1 );
        connect( stringEdit, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
        connect( stringEdit, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(settingsChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_ENUM:
    {
        enumEdit = new KComboBox( parent );
        for( int i = 0; entry->enum_values[i]; ++i )
            enumEdit->insertItem( entry->enum_values[i] );
        enumEdit->setCurrentItem( m_numValue );
        grid->addWidget( enumEdit, row, 1 );
        connect( enumEdit, SIGNAL(activated( int )), this, SLOT(slotNumChanged( int )) );
        connect( enumEdit, SIGNAL(activated( int )), pluginConfig, SIGNAL(settingsChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_NUM:
    {
        numEdit = new QSpinBox( -999999, 999999, 1, parent );
        numEdit->setValue( m_numValue );
        grid->addWidget( numEdit, row, 1 );
        connect( numEdit, SIGNAL(valueChanged( int )), this, SLOT(slotNumChanged( int )) );
        connect( numEdit, SIGNAL(valueChanged( int )), pluginConfig, SIGNAL(settingsChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_RANGE:
    {
        numEdit = new QSpinBox( parent );
        numEdit->setValue( m_numValue );
        numEdit->setRange( entry->range_min, entry->range_max );
        grid->addWidget( numEdit, row, 1 );
        connect( numEdit, SIGNAL(valueChanged( int )), this, SLOT(slotNumChanged( int )) );
        connect( numEdit, SIGNAL(valueChanged( int )), pluginConfig, SIGNAL(settingsChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_BOOL:
    {
        boolEdit = new QCheckBox( parent );
        boolEdit->setChecked( m_numValue );
        grid->addWidget( boolEdit, row, 1 );
        connect( boolEdit, SIGNAL(toggled( bool )), this, SLOT(slotBoolChanged( bool )) );
        connect( boolEdit, SIGNAL(toggled( bool )), pluginConfig, SIGNAL(settingsChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_UNKNOWN:
        ;
    }

    QLabel* description = new QLabel( QString::fromUtf8( entry->description ) + ':', parent );
    description->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );

    grid->addWidget( description, row, 0 );
}

inline void
XineConfigEntry::slotNumChanged( int val )
{
    m_numValue     = val;
    m_valueChanged = true;
}

inline void
XineConfigEntry::slotBoolChanged( bool val )
{
    m_numValue     = val;
    m_valueChanged = true;
}

inline void
XineConfigEntry::slotStringChanged( const QString& val )
{
    m_stringValue  = val.utf8();
    m_valueChanged = true;
}


///////////////////////
/// XineConfigDialog
///////////////////////

XineConfigDialog::XineConfigDialog( const xine_t* const xine, QWidget *p )
    : amaroK::PluginConfig()
    , QTabWidget( p )
    , m_xine( (xine_t*)xine )
{
    int row = 0;
    QString currentPage;
    QWidget *parent = 0;
    QGridLayout *grid = 0;
    xine_cfg_entry_t entry;
    xine_cfg_entry_t *ent = &entry;
    QScrollView *sv = 0;

    xine_config_get_first_entry( m_xine, ent );

    do
    {
        if ( ent->exp_level <= 20 )
        {
            QString pageName( ent->key );
            pageName = pageName.left( pageName.find( '.' ) );

            // if (pageName == "decoder") continue;

            if ( pageName == currentPage )
            {
                ++row;
            }
            else
            {
                if ( sv )
                    //TODO is the viewport() not better?
                    sv->setMinimumWidth( grid->sizeHint().width() + 20 );

                addTab( sv = new QScrollView, pageName );
                parent = new QWidget( sv->viewport() );

                sv->setResizePolicy( QScrollView::AutoOneFit );
                sv->setHScrollBarMode( QScrollView::AlwaysOff );
                sv->setFrameShape( QFrame::NoFrame );
                sv->addChild( parent );

                grid = new QGridLayout( parent, /*rows*/20, /*cols*/2, /*margin*/10, /*spacing*/10 );
                grid->setColStretch( 0, 3 );
                grid->setColStretch( 1, 1 );

                currentPage = pageName;
                row = 0;
            }

            entrys.append( new XineConfigEntry( parent, this, row, ent ) );
            ++row;
            grid->addMultiCellWidget( new KSeparator( KSeparator::Horizontal, parent ), row, row, 0, 1 );
        }
    }
    while( xine_config_get_next_entry( m_xine, ent ) );

    entrys.setAutoDelete( true );
}

bool
XineConfigDialog::hasChanged() const
{
    for( QPtrListIterator<XineConfigEntry> it( entrys ); *it != 0; ++it )
        if ( (*it)->isChanged() )
            return true;

    return false;
}

bool
XineConfigDialog::isDefault() const
{
    return false;
}

void
XineConfigDialog::save()
{
    xine_cfg_entry_t ent;

    for( XineConfigEntry *entry = entrys.first(); entry; entry = entrys.next() )
    {
        if( entry->isChanged() &&
            xine_config_lookup_entry( m_xine, entry->key(), &ent ) )
        {
            kdDebug() << "[xine-engine] Apply: " << entry->key() << "\n";

            ent.num_value = entry->numValue();

            if ( entry->stringValue() )
                ent.str_value = (char*) (const char*)entry->stringValue();

            xine_config_update_entry( m_xine, &ent );

            entry->setUnchanged();
        }
    }
}

#include "xine-config.moc"
