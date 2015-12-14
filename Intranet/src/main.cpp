#include "defaultrequesthandler.h"

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QSettings>

#include <httplistener.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("KFG Intranet Server");

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption dataDirOption(QStringList() << "d" << "data", "The data dir.", "data",
                                 #ifdef QT_DEBUG
                                     "intranet-data"
                                 #else
                                     "/usr/share/intranet/data"
                                 #endif
                                     );
    parser.addOption(dataDirOption);
    parser.addPositionalArgument("config", "The configuration file for the server.", "<config>");
    parser.process(app);
    QStringList args = parser.positionalArguments();

    QString dataDirName(parser.value(dataDirOption));
    qDebug() << "Using data dir" << dataDirName;
    QDir dataDir(dataDirName);
    if (!dataDir.exists())
    {
        QFileInfo dataInfo(dataDirName);
        if (!dataInfo.dir().mkdir(dataInfo.fileName()))
        {
            qCritical("Failed to create the data dir!");
            return 1;
        }
    }

    QSettings *config = new QSettings(args.size()>0 ? args[0] :
                                  #ifdef QT_DEBUG
                                      "config.ini"
                                  #else
                                      "/etc/intranet/config.ini"
                                  #endif
                                      , QSettings::IniFormat);
    qDebug() << "Using config file " << config->fileName();
    new HttpListener(config, new DefaultRequestHandler);

    return app.exec();
}

