/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void Options5::init()
{
  m_pOSDPreview = 0;
  int numScreens = QApplication::desktop()->numScreens();
  for( int i = 0; i < numScreens; i++ )
      kcfg_OsdScreen->insertItem( QString::number( i ) );
}

void Options5::destroy()
{
    delete m_pOSDPreview;
}

void Options5::fontChanged(const QFont &font )
{
    if( m_pOSDPreview )
        m_pOSDPreview->setFont( font );
}


void Options5::textColorChanged( const QColor &color )
{
    if( m_pOSDPreview )
        m_pOSDPreview->setTextColor( color );
}


void Options5::backgroundColorChanged( const QColor &color )
{
    if( m_pOSDPreview )
        m_pOSDPreview->setBackgroundColor( color );
}


void Options5::alignmentChanged( int alignment )
{
    if( m_pOSDPreview )
        m_pOSDPreview->setPosition( (OSDWidget::Position)alignment );
}


void Options5::screenChanged( int screen )
{
    if( m_pOSDPreview )
        m_pOSDPreview->setScreen( screen );
}


void Options5::xOffsetChanged( int offset )
{
    if( m_pOSDPreview )
    {
        m_pOSDPreview->setOffset( offset, kcfg_OsdYOffset->value() );
    }
}


void Options5::yOffsetChanged( int offset )
{
    if( m_pOSDPreview )
    {
        m_pOSDPreview->setOffset( kcfg_OsdXOffset->value(), offset );
    }
}


void Options5::preview( bool on )
{
    if( on )
    {
        if( !m_pOSDPreview )
        {
            m_pOSDPreview = new OSDPreviewWidget( i18n( "amaroK" ) );
            connect( m_pOSDPreview, SIGNAL( positionChanged( int, OSDWidget::Position, int, int ) ) ,
                     this, SLOT( osdPositionChanged( int, OSDWidget::Position, int, int ) ) );
        }

        m_pOSDPreview->setFont( kcfg_OsdFont->font() );
        m_pOSDPreview->setTextColor( kcfg_OsdTextColor->color() );
        m_pOSDPreview->setBackgroundColor( kcfg_OsdBackgroundColor->color() );
        m_pOSDPreview->setPosition( (OSDWidget::Position)kcfg_OsdAlignment->currentItem() );
        m_pOSDPreview->setScreen( kcfg_OsdScreen->currentItem() );
        m_pOSDPreview->setOffset( kcfg_OsdXOffset->value(), kcfg_OsdYOffset->value() );
        m_pOSDPreview->showOSD( i18n("OSD preview - The quick brown fox jumps over the lazy dog" ) );
    }
    else
    {
        if( m_pOSDPreview )
            m_pOSDPreview->hide();
    }
}

void Options5::osdPositionChanged( int screen, OSDWidget::Position alignment, int xOffset, int yOffset )
{
    // atomic
    kcfg_OsdScreen->blockSignals( true );
    kcfg_OsdAlignment->blockSignals( true );
    kcfg_OsdXOffset->blockSignals( true );
    kcfg_OsdYOffset->blockSignals( true );

    kcfg_OsdScreen->setCurrentItem( screen );
    kcfg_OsdAlignment->setCurrentItem( alignment );
    kcfg_OsdXOffset->setValue( xOffset );
    kcfg_OsdYOffset->setValue( yOffset );

    kcfg_OsdScreen->blockSignals( false );
    kcfg_OsdAlignment->blockSignals( false );
    kcfg_OsdXOffset->blockSignals( false );
    kcfg_OsdYOffset->blockSignals( false );
}


void Options5::hideEvent( QHideEvent * )
{
    if( m_pOSDPreview )
    {
        delete m_pOSDPreview;
        m_pOSDPreview = 0;
        PreviewButton->setOn( false );
    }
}


void Options5::osdEnabledChange( bool on )
{
    if( on == false && m_pOSDPreview )
    {
        delete m_pOSDPreview;
        m_pOSDPreview = 0;
        PreviewButton->setOn( false );
    }
}
