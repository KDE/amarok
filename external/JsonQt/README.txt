The different Interfaces
------------------------

The most basic interface is JsonToVariant/VariantToJson. These classes map JSON to and
from QVariants.

A JSON object is mapped to a QVariantMap, arrays to QVariantList, null to a null QVariant,
and values to bools, ints, floats, doubles, and QStrings as appropriate.



There's also JsonToProperties - this takes a JSON object, and calls QObject::setProperty() for
each value; for example:

{"foo"->"bar"} would call yourObject->setProperty("foo", "bar");

These properties are readable via QObject::property(), or via member variables/accessors if the
property was declared with Q_PROPERTY.



JsonRpc is a basic parser for JSON-RPC 2.0 - this is abstracted slightly by JsonRpcAdaptor, which
provides a simple interface for exposing a QObject's public slots via JSON-RPC 2.0.

Using in your project
---------------------

It's easiest if you just install this system-wide, and use it like any other library.

In a cmake project, you can statically include it with something like the following:

# JsonQt
FIND_LIBRARY(JsonQt JsonQt)
IF("${JsonQt}" STREQUAL "JsonQt-NOTFOUND")
        MESSAGE(STATUS "System JsonQt not found, using bundlded version.")
        # Don't build JsonQt tests
        SET(BUILD_TESTS OFF)
        SET(STATIC_JSONQT ON)
        ADD_SUBDIRECTORY(JsonQt)
        SET(JsonQt JsonQt PARENT_SCOPE)
ENDIF()

FIND_PATH(JSONQT_INCLUDE_DIR VariantToJson.h PATHS "${CMAKE_CURRENT_SOURCE_DIR}/JsonQt/lib")
