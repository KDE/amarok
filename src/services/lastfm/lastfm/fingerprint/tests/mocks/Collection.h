#ifndef MOCK_COLLECTION_H
#define MOCK_COLLECTION_H
#ifdef COLLECTION_H
    #error The collection declaration must come before the mock object
#endif

#include "../../Collection.h"

class MockCollection : public Collection
{
public:
    QString
    getFingerprint( const QString& filePath )
    {
        qDebug() << "Using mock collection wohoo!";
        Q_UNUSED( filePath );
        if( fingerprintFound )
            return "123";
        
        return "";
    }
    
    static bool fingerprintFound;
};

bool MockCollection::fingerprintFound = false;

#define Collection MockCollection
#endif //MOCK_COLLECTION_H
