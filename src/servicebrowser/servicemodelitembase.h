#ifndef AMAROKSERVICEMODELITEMBASE_H
#define AMAROKSERVICEMODELITEMBASE_H 

#include <QString>
#include <QVariant>

class ServiceModelItemBase
{
public:

    virtual QVariant data(int column) const = 0;

    ServiceModelItemBase *child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    ServiceModelItemBase * parent();
    QList<ServiceModelItemBase*> getChildItems() const;
    bool hasChildren () const;
    QString getUrl();


protected:
     mutable QList<ServiceModelItemBase*> m_childItems;
     ServiceModelItemBase *m_parent;;
};

#endif