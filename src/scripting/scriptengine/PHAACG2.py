# TODOs:
# NO enums like in AmarokInfoScript.cpp handled, global func not handled
# handle enums in scriptdox
# Handle multiple classes in a file
# Handle docs in script dox

import os
import sys
import re

prototype = {}
scriptMap = {}
autoComplete = []

def addElement( scriptMap, name, classDoc = '' ):
    #if not scriptMap.has_key( name ):
    scriptMap[name] = ["\nclass " + name + "{\n"]

def generatePseudoHeader( rootDir, fileName ):
    global prototype
    global scriptMap
    global autoComplete

    headerPath = os.path.join(rootDir, fileName+".h")

    className = None
    with open( headerPath, 'r' ) as f:
        contents = f.read()
        match = re.search( r'//[\s]*SCRIPTDOX[\s:]*(.*)', contents )
        if match is None:
            return
        s = filter( None, match.group(1).split(' ') )
        if 'PROTOTYPE' in s:
            className = s[2].strip()
            prototype[ s[1] ] = className
        else:
            className = s[0].strip()
            autoComplete.append( className )
        addElement( scriptMap, className )#, classDoc )
        classBeg = contents.index(match.group(0))
        if True:
            properties = re.findall( r"(Q_PROPERTY\(\s*[\w:<>]*\s*([\w:<>\*]*).*\))", contents, re.MULTILINE )
            for qProperty in properties:
                if 'PROTOTYPE' not in s:
                    autoComplete.append( className + "." + qProperty[1] )
                scriptMap[className].append( qProperty[0] )
            #re.findall( r"public slots:[\s\n]*(.*)[\n\s]*(protected|private|public|slots|};|signals):" #huh, works without |signals?!
            #            , contents, re.MULTILINE|re.DOTALL )
            accessMap = {}
            for access in ( 'private slots:', 'protected slots:', 'public slots:', 'private:', 'public:', 'protected:', 'signals:', '};' ):
                indexFromBeg = contents[classBeg:].find( access ) + classBeg
                if indexFromBeg is not -1:
                    accessMap[ indexFromBeg ] = access

            sortedKeys = sorted( accessMap.keys() )
            for access in ('public slots:', 'signals:'):
                if( access in contents ):
                    accessIndex = contents[classBeg:].find( access ) + classBeg
                    selections = contents[ accessIndex : sortedKeys[sortedKeys.index(accessIndex)] ]
                    functions = filter( None, re.findall( r'(.*;)', selections ) )
                    if 'PROTOTYPE' not in s:
                        for function in functions:
                             reg = re.search( r'.*[\s\n](.*)[\s\n]*\(', function.strip())
                             if reg:
                                funcName = reg.group(1).replace('*','').replace('&','')
                                autoComplete.append( className+"."+ funcName + '(' )
                    scriptMap[className].append( selections )

def traverseDir( dirPath, opDir ):
    for root, subFolder, files in os.walk(dirPath):
        for fileName in files:
            if fileName.endswith( ".cpp" ):
                generatePseudoHeader( root, fileName[:-4] )
    #substitute prototypes
    for scriptClass in scriptMap:
        for line in scriptMap[scriptClass]:
            re.sub( r"\b("+'|'.join(prototype.keys())+r')\b', lambda word: prototype[word.group()], line )

    f = open( os.path.join( opDir, "PseudoHeader.h" ), 'w' )
    for scriptClass in scriptMap:
        f.write( '\n'.join(scriptMap[scriptClass]) + '};\n' )
    f.close()

    autoComplete.sort()
    f = open( os.path.join( opDir, "AutoComplete.txt" ), 'w' )
    f.write( '\n'.join(autoComplete) )
    f.close()

def main():
    scriptPath = sys.argv[1]
    if not os.path.exists(sys.argv[2]):
        os.makedirs(sys.argv[2])
    traverseDir( sys.argv[1], sys.argv[2] )

if __name__ == "__main__":
    main()
