#include "defaultrequesthandler.h"

DefaultRequestHandler::DefaultRequestHandler(QObject *parent)
    : HttpRequestHandler(parent)
    , staticFiles(new QSettings(":/static/static.ini", QSettings::IniFormat))
    , templates(new QSettings(":/html/html.ini", QSettings::IniFormat))
{
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
            base.setVariable("body", t);
    }
    response.write(base.toUtf8(), true);
}
