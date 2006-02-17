#ifndef HINTLINEEDIT_H
#define HINTLINEEDIT_H

#include <klineedit.h> //baseclass

class QVBox;
class QLabel;
class QWidget;

class HintLineEdit : public KLineEdit
{
    Q_OBJECT

public:
    HintLineEdit( const QString &hint, const QString &text, QWidget *parent = 0, const char *name = 0 );
    HintLineEdit( const QString &text, QWidget *parent = 0, const char *name = 0 );
    HintLineEdit( QWidget *parent = 0, const char *name = 0 );
    virtual ~HintLineEdit();
    virtual QObject *parent();
    virtual void setHint( const QString &hint );
private:
    void init();
    QVBox *m_vbox;
    QLabel *m_hint;
};

#endif
