#include <hintlineedit.h>
#include <q3vbox.h>
#include <QLabel>
#include <qfont.h>

HintLineEdit::HintLineEdit( const QString &hint, const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new Q3VBox( parent ) )
{
    init();
    m_hint->setText( hint );
}

HintLineEdit::HintLineEdit( const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new Q3VBox( parent ) )
{
    init();
}

HintLineEdit::HintLineEdit( QWidget *parent )
   : KLineEdit( 0 )
   , m_vbox( new Q3VBox( parent ) )
{
    init();
}

void
HintLineEdit::init()
{
    reparent( m_vbox, 0, QPoint(0,0), true );
    m_hint = new QLabel( m_vbox );
    //m_hint->setBuddy( this );
    m_hint->setFocusPolicy( Qt::NoFocus );
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
