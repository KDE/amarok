
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "debug.h"
#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kseparator.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <xine.h>
#include "xine-config.h"


XineConfigEntry::XineConfigEntry( QWidget *parent, amaroK::PluginConfig *pluginConfig, int row, xine_cfg_entry_t *entry )
    : m_valueChanged( false )
    , m_numValue( entry->num_value )
    , m_key( entry->key )
    , m_stringValue( entry->str_value )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();
    QWidget *w = 0;

    switch (entry->type)
    {
    case XINE_CONFIG_TYPE_STRING:
    {
        w = new KLineEdit( m_stringValue, parent );
        connect( w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
        connect( w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_ENUM:
    {
        KComboBox *box = new KComboBox( parent );
        w = box;
        for( int i = 0; entry->enum_values[i]; ++i )
            box->insertItem( entry->enum_values[i] );
        box->setCurrentItem( m_numValue );
        connect( w, SIGNAL(activated( int )), this, SLOT(slotNumChanged( int )) );
        connect( w, SIGNAL(activated( int )), pluginConfig, SIGNAL(viewChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_NUM:
    {
        w = new QSpinBox( entry->range_min, entry->range_max, 1, parent );
        ((QSpinBox*)w)->setValue( m_numValue );
        connect( w, SIGNAL(valueChanged( int )), this, SLOT(slotNumChanged( int )) );
        connect( w, SIGNAL(valueChanged( int )), pluginConfig, SIGNAL(viewChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_RANGE:
    {
        QSpinBox *box = new QSpinBox( parent );
        w = box;
        box->setValue( m_numValue );
        box->setRange( entry->range_min, entry->range_max );
        connect( w, SIGNAL(valueChanged( int )), this, SLOT(slotNumChanged( int )) );
        connect( w, SIGNAL(valueChanged( int )), pluginConfig, SIGNAL(viewChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_BOOL:
    {
        QCheckBox *box = new QCheckBox( parent );
        w = box;
        box->setChecked( m_numValue );
        connect( w, SIGNAL(toggled( bool )), this, SLOT(slotBoolChanged( bool )) );
        connect( w, SIGNAL(toggled( bool )), pluginConfig, SIGNAL(viewChanged()) );
        break;
    }
    case XINE_CONFIG_TYPE_UNKNOWN:
        ;
    }

    QToolTip::add( w, QString( entry->help ) );

    QLabel* description = new QLabel( QString( entry->description ) + ':', parent );
    description->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );

    grid->addWidget( w, row, 1 );
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
        if ( ent->exp_level <= 10 )
        {
            QString pageName( ent->key );
            pageName = pageName.left( pageName.find( '.' ) );

            if ( pageName == "vcd" )
                continue;

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
      if( entry->isChanged() && xine_config_lookup_entry( m_xine, entry->key(), &ent ) )
      {
         debug() << "Apply: " << entry->key() << "\n";

         ent.num_value = entry->numValue();

         if( entry->stringValue() )
            ent.str_value = (char*) (const char*)entry->stringValue();

         xine_config_update_entry( m_xine, &ent );

         entry->setUnchanged();
      }
   }
}

#include "xine-config.moc"
