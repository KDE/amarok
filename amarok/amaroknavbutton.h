#ifndef AMAROKNAVBUTTON_H
#define AMAROKNAVBUTTON_H

#include <qpushbutton.h>

class QPainter;

/**
 * @brief: The player navigation button, with OwnerDraw code
 */

class AmarokNavButton : public QPushButton
{
    Q_OBJECT

    public:
        AmarokNavButton( QWidget *parent, const char *name = 0 ) : QPushButton(parent, name) {};
        ~AmarokNavButton() {};

    protected:
	void drawButton ( QPainter* );
};

#endif
