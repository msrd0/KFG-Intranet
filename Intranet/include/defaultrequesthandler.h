#ifndef DEFAULTREQUESTHANDLER_H
#define DEFAULTREQUESTHANDLER_H

#include <QDir>
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
	
    DefaultRequestHandler(const QDir &dataDir, QObject *parent = 0);

    void service(HttpRequest& request, HttpResponse& response);

private:
	HttpSessionStore sessionStore;
    StaticFileController staticFiles;
    TemplateCache templates;

    QSqlDatabase *db;
    bool exec(const QString &query);

};

#endif // DEFAULTREQUESTHANDLER_H
