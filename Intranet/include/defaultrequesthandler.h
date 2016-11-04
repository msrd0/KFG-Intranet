#ifndef DEFAULTREQUESTHANDLER_H
#define DEFAULTREQUESTHANDLER_H

#include "intranet.h"
#include "db_intranet.h"

#include <QDir>
#include <QMutex>

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

    qsl::db::intranet d;
	
	QMutex helpmdmutex;
	QFile helpmdfile;
	QByteArray helpmd;

};

#endif // DEFAULTREQUESTHANDLER_H
