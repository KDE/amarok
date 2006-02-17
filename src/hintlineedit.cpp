#include <hintlineedit.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qfont.h>

HintLineEdit::HintLineEdit( const QString &hint, const QString &text, QWidget *parent, const char *name )
   : KLineEdit( text, 0, name )
   , m_vbox( new QVBox( parent ) )
{
    init();
    m_hint->setText( hint );
}

HintLineEdit::HintLineEdit( const QString &text, QWidget *parent, const char *name )
   : KLineEdit( text, 0, name )
   , m_vbox( new QVBox( parent ) )
{
    init();
}

HintLineEdit::HintLineEdit( QWidget *parent, const char *name )
   : KLineEdit( 0, name )
   , m_vbox( new QVBox( parent ) )
{
    init();
}

void
HintLineEdit::init()
{
    reparent( m_vbox, 0, QPoint(0,0), true );
    m_hint = new QLabel( m_vbox );
    //m_hint->setBuddy( this );
    m_hint->setFocusPolicy( NoFocus );
    QFont font;
    font.setPointSize( font.pointSize() - 2);
    m_hint->setFont( font );
}

HintLineEdit::~HintLineEdit()
{
    reparent( 0, 0, QPoint(0,0), false );
    delete m_vbox;
}

void
HintLineEdit::setHint( const QString &hint )
{
    m_hint->setText( hint );
}

QObject *
HintLineEdit::parent()
{
    return m_vbox->parent();
}

#include "hintlineedit.moc"
