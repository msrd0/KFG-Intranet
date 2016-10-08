#ifndef DEFAULTREQUESTHANDLER_H
#define DEFAULTREQUESTHANDLER_H

#include <QDir>
#include <QMutex>
#include <QSqlDatabase>

#include <httprequesthandler.h>
#include <httpsessionstore.h>
#include <staticfilecontroller.h>
#include <templatecache.h>

class DefaultRequestHandler : public HttpRequestHandler
{
    Q_OBJECT

public:
	struct Item
	{
		QString name;
		QString img;
		QString link;
	};
	
    DefaultRequestHandler(const QDir &sharedDir, const QDir &dataDir, const QByteArray &prep, QObject *parent = 0);

    void service(HttpRequest& request, HttpResponse& response);

private:
	QDir sharedDir;
	QByteArray prepend;
	
	HttpSessionStore sessionStore;
    StaticFileController staticFiles;
    TemplateCache templates;

    QSqlDatabase *db;
    bool exec(const QString &query);
	
	QMutex helpmdmutex;
	QFile helpmdfile;
	QByteArray helpmd;

};

#endif // DEFAULTREQUESTHANDLER_H
