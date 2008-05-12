#include "hintlineedit.h"

#include <kvbox.h>

#include <QFont>
#include <QLabel>

HintLineEdit::HintLineEdit( const QString &hint, const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new KVBox( parent ) )
{
    init();
    m_hint->setText( hint );
}

HintLineEdit::HintLineEdit( const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new KVBox( parent ) )
{
    init();
}

HintLineEdit::HintLineEdit( QWidget *parent )
   : KLineEdit( 0 )
   , m_vbox( new KVBox( parent ) )
{
    init();
}

void
HintLineEdit::init()
{
    setParent( m_vbox );
    show();
    m_hint = new QLabel( m_vbox );
    //m_hint->setBuddy( this );
    m_hint->setFocusPolicy( Qt::NoFocus );
    QFont font;
    font.setPointSize( font.pointSize() - 2);
    m_hint->setFont( font );
}

HintLineEdit::~HintLineEdit()
{
    setParent( 0 );
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
