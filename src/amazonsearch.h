// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAZONSEARCH_H
#define AMAZONSEARCH_H

#include <qvariant.h>
#include <qpixmap.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QComboBox;
class KLineEdit;
class QPushButton;
class QLabel;

class AmazonSearch : public QDialog
{
    Q_OBJECT

public:
    AmazonSearch( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~AmazonSearch();

    KLineEdit* searchString;
    QComboBox* searchSite;
    QPushButton* cancelButton;
    QPushButton* okButton;
    QPushButton* fileButton;
    QLabel* textLabel;

signals:
        void imageReady( QPixmap image );        

public slots:
    virtual void openFile();

protected:
    QGridLayout* AmazonSearchLayout;
    QSpacerItem* spacer3;

private:
    
};

#endif // AMAZONSEARCH_H
