#ifndef HINTLINEEDIT_H
#define HINTLINEEDIT_H

#include <klineedit.h> //baseclass
//Added by qt3to4:
#include <QLabel>

class KVBox;
class QLabel;
class QWidget;

class HintLineEdit : public KLineEdit
{
    Q_OBJECT

public:
    HintLineEdit( const QString &hint, const QString &text, QWidget *parent = 0 );
    explicit HintLineEdit( const QString &text, QWidget *parent = 0 );
    HintLineEdit( QWidget *parent = 0 );
    virtual ~HintLineEdit();
    virtual QObject *parent();
    virtual void setHint( const QString &hint );
private:
    void init();
    KVBox *m_vbox;
    QLabel *m_hint;
};

#endif
