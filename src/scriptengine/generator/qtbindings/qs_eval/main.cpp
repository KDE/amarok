/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the <your project> project on Trolltech Labs.
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


#include <QtScript>
#if QT_VERSION >= 0x040500
#include <QtScriptTools>
#endif
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtGui/QApplication>

#include <stdlib.h>

static bool wantsToQuit;

static QScriptValue qtscript_quit(QScriptContext *ctx, QScriptEngine *eng)
{
    Q_UNUSED(ctx);
    wantsToQuit = true;
    return eng->undefinedValue();
}

static QScriptValue qtscript_describe(QScriptContext *ctx, QScriptEngine *eng)
{
    QStringList result;
    QScriptValue obj = ctx->argument(0);
    while (obj.isObject()) {
        QScriptValueIterator it(obj);
        while (it.hasNext()) {
            it.next();
            result.append(it.name());
        }
        obj = obj.prototype();
    }
    return eng->toScriptValue(result);
}

static void interactive(QScriptEngine *eng)
{
    QScriptValue global = eng->globalObject();
    QScriptValue quitFunction = eng->newFunction(qtscript_quit);
    if (!global.property(QLatin1String("exit")).isValid())
        global.setProperty(QLatin1String("exit"), quitFunction);
    if (!global.property(QLatin1String("quit")).isValid())
        global.setProperty(QLatin1String("quit"), quitFunction);
    wantsToQuit = false;
    global.setProperty(QLatin1String("describe"),
                       eng->newFunction(qtscript_describe));
   
    QTextStream qin(stdin, QFile::ReadOnly);

    const char *qscript_prompt = "qs> ";
    const char *dot_prompt = ".... ";
    const char *prompt = qscript_prompt;

    QString code;

    forever {
        QString line;

        printf("%s", prompt);
        fflush(stdout);

        line = qin.readLine();
        if (line.isNull())
            break;

        code += line;
        code += QLatin1Char('\n');

        if (line.trimmed().isEmpty()) {
            continue;

        } else if (! eng->canEvaluate(code)) {
            prompt = dot_prompt;

        } else {
            QScriptValue result = eng->evaluate(code, QLatin1String("typein"));

            code.clear();
            prompt = qscript_prompt;

            if (! result.isUndefined())
                fprintf(stderr, "%s\n", qPrintable(result.toString()));

            if (wantsToQuit)
                break;
        }
    }
}

static QScriptValue importExtension(QScriptContext *context, QScriptEngine *engine)
{
    return engine->importExtension(context->argument(0).toString());
}

int main(int argc, char *argv[])
{
    QApplication *app;
    if (argc >= 2 && !qstrcmp(argv[1], "-tty"))
        app = new QApplication(argc, argv, QApplication::Tty);
    else
        app = new QApplication(argc, argv);

    QDir dir(QApplication::applicationDirPath());
    if (dir.dirName() == QLatin1String("debug") || dir.dirName() == QLatin1String("release"))
        dir.cdUp();
    dir.cdUp();
    dir.cdUp();
    if (!dir.cd("plugins")) {
        fprintf(stderr, "plugins folder does not exist -- did you build the bindings?\n");
        return(-1);
    }
    QStringList paths = app->libraryPaths();
    paths <<  dir.absolutePath();
    app->setLibraryPaths(paths);

    QScriptEngine *eng = new QScriptEngine();
#if QT_VERSION >= 0x040500
    QScriptEngineDebugger *dbg = new QScriptEngineDebugger();
    dbg->attachTo(eng);
#endif

    eng->importExtension("qt.core");
    eng->importExtension("qt.gui");
    eng->importExtension("qt.xml");
    eng->importExtension("qt.svg");
    eng->importExtension("qt.network");
    eng->importExtension("qt.sql");
    eng->importExtension("qt.opengl");
    eng->importExtension("qt.webkit");
    eng->importExtension("qt.xmlpatterns");
    eng->importExtension("qt.uitools");

    QScriptValue globalObject = eng->globalObject();
    globalObject.setProperty("qApp", eng->newQObject(app));
    {
        QScriptValue qscript = eng->newObject();
        qscript.setProperty("importExtension", eng->newFunction(importExtension));
        globalObject.property("qt").setProperty("script", qscript);
    }

    if (! *++argv) {
        interactive(eng);
        return EXIT_SUCCESS;
    }

    while (const char *arg = *argv++) {
        QString fn = QString::fromLocal8Bit(arg);

        if (fn == QLatin1String("-i")) {
            interactive(eng);
            break;
        }

        QString contents;
        int lineNumber = 1;

        if (fn == QLatin1String("-")) {
            QTextStream stream(stdin, QFile::ReadOnly);
            contents = stream.readAll();
        }

        else {
            QFile file(fn);

            if (file.open(QFile::ReadOnly)) {
                QTextStream stream(&file);
                contents = stream.readAll();
                file.close();

                // strip off #!/usr/bin/env qscript line
                if (contents.startsWith("#!")) {
                    contents.remove(0, contents.indexOf("\n"));
                    ++lineNumber;
                }
            }
        }

        if (contents.isEmpty())
            continue;

        QScriptValue r = eng->evaluate(contents, fn, lineNumber);
        if (eng->hasUncaughtException()) {
            QStringList backtrace = eng->uncaughtExceptionBacktrace();
            fprintf (stderr, "    %s\n%s\n\n", qPrintable(r.toString()),
                     qPrintable(backtrace.join("\n")));
            return EXIT_FAILURE;
        }
    }

    delete eng;
#if QT_VERSION >= 0x040500
    delete dbg;
#endif
    delete app;

    return EXIT_SUCCESS;
}
