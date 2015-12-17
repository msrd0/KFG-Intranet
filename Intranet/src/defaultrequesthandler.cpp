#include "defaultrequesthandler.h"

#include <QSqlError>
#include <QSqlQuery>

#define CREATE_TABLE(name, sql) \
    if (!exec("CREATE TABLE IF NOT EXISTS '" name "' (" sql ");")) \
    { \
        db->close(); \
        delete db; \
        db = 0; \
    }

DefaultRequestHandler::DefaultRequestHandler(const QDir &dataDir, QObject *parent)
    : HttpRequestHandler(parent)
    , staticFiles(new QSettings(":/static/static.ini", QSettings::IniFormat))
    , templates(new QSettings(":/html/html.ini", QSettings::IniFormat))
{
    db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    db->setDatabaseName(dataDir.absoluteFilePath("db-v1")); // may increase db version in future
    if (!db->open())
        db = 0;
    else
    {
        CREATE_TABLE("admins",
                     "password TEXT");
        CREATE_TABLE("rows",
                     "id INTEGER PRIMARY KEY UNIQUE,"
                     "name TEXT UNIQUE");
        CREATE_TABLE("items",
                     "row INTEGER,"
                     "name TEXT UNIQUE,"
                     "FOREIGN KEY(row) REFERENCES rows(id)");
        CREATE_TABLE("news",
                     "text TEXT,"
                     "edited DATETIME");
    }
}

bool DefaultRequestHandler::exec(const QString &query)
{
#ifdef QT_DEBUG
    qDebug() << query;
#endif
    if (db == 0)
    {
        qCritical("No database available");
        return false;
    }
    QSqlQuery q = db->exec(query);
    if (q.lastError().isValid())
    {
        qCritical() << q.lastError();
        return false;
    }
    return true;
}

void DefaultRequestHandler::service(HttpRequest &request, HttpResponse &response)
{
    QByteArray path = request.getPath().mid(1);
    response.setHeader("Server", QByteArray("KFG-Intranet (QtWebApp ") + getQtWebAppLibVersion() + ")");

    if (path == "")
    {
        response.redirect("index");
        return;
    }

    if (path.startsWith("static/"))
    {
        staticFiles.service(request, response);
        return;
    }

    response.setHeader("Content-Type", "text/html; charset=utf-8");
    Template base = templates.getTemplate("base");
    base.setVariable("name", path);
    if (path == "base")
    {
        response.setStatus(403, "Forbidden");
        base.setVariable("body", "403 Forbidden");
    }
    else
    {
        Template t = templates.getTemplate(path);
        if (t.isEmpty())
        {
            response.setStatus(404, "Not Found");
            base.setVariable("body", "404 Not Found");
        }
        else
        {
            //
            base.setVariable("body", t);
        }
    }
    response.write(base.toUtf8(), true);
}
