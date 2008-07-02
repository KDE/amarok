/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Script Generator project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "shellheadergenerator.h"
#include "fileout.h"

#include <QtCore/QDir>

#include <qdebug.h>

QString ShellHeaderGenerator::fileNameForClass(const AbstractMetaClass *meta_class) const
{
    return QString("qtscriptshell_%1.h").arg(meta_class->name());
}

void writeQtScriptQtBindingsLicense(QTextStream &stream);

void ShellHeaderGenerator::write(QTextStream &s, const AbstractMetaClass *meta_class)
{
    if (FileOut::license)
        writeQtScriptQtBindingsLicense(s);

    QString include_block = "QTSCRIPTSHELL_" + meta_class->name().toUpper() + "_H";

    s << "#ifndef " << include_block << endl
      << "#define " << include_block << endl << endl;

    Include inc = meta_class->typeEntry()->include();
    s << "#include ";
    if (inc.type == Include::IncludePath)
        s << "<";
    else
        s << "\"";
    s << inc.name;
    if (inc.type == Include::IncludePath)
        s << ">";
    else
        s << "\"";
    s << endl << endl;

    s << "#include <QtScript/qscriptvalue.h>" << endl << endl;

    QString pro_file_name = meta_class->package().replace(".", "_") + "/" + meta_class->package().replace(".", "_") + ".pri";

    if (!meta_class->generateShellClass()) {
        s << "#endif" << endl << endl;
        priGenerator->addHeader(pro_file_name, fileNameForClass(meta_class));
        return ;
    }

    s << "class " << shellClassName(meta_class)
      << " : public " << meta_class->qualifiedCppName() << endl
      << "{" << endl;


    s << "public:" << endl;
    foreach (const AbstractMetaFunction *function, meta_class->functions()) {
        if (function->isConstructor() && !function->isPrivate()) {
            s << "    ";
            writeFunctionSignature(s, function, 0, QString(),
                                   Option(IncludeDefaultExpression | OriginalName | ShowStatic));
            s << ";" << endl;
        }
    }

    s << "    ~" << shellClassName(meta_class) << "();" << endl;
    s << endl;

    AbstractMetaFunctionList functions = meta_class->queryFunctions(
        AbstractMetaClass:: VirtualFunctions | AbstractMetaClass::WasVisible
        | AbstractMetaClass::NotRemovedFromTargetLang
        );

    for (int i = 0; i < functions.size(); ++i) {
        s << "    ";
        writeFunctionSignature(s, functions.at(i), 0, QString(),
                               Option(IncludeDefaultExpression | OriginalName | ShowStatic | UnderscoreSpaces));
        s << ";" << endl;
    }

    writeInjectedCode(s, meta_class);

    s  << endl << "    QScriptValue __qtscript_self;" << endl;

    s  << "};" << endl << endl
       << "#endif // " << include_block << endl;

    priGenerator->addHeader(pro_file_name, fileNameForClass(meta_class));
}

void ShellHeaderGenerator::writeInjectedCode(QTextStream &s, const AbstractMetaClass *meta_class)
{
    CodeSnipList code_snips = meta_class->typeEntry()->codeSnips();
    foreach (const CodeSnip &cs, code_snips) {
        if (cs.language == TypeSystem::ShellDeclaration) {
            s << cs.code() << endl;
        }
    }
}
