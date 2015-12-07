#include "defaultrequesthandler.h"

DefaultRequestHandler::DefaultRequestHandler(QObject *parent)
    : HttpRequestHandler(parent)
    , templates(new QSettings(":/html/html.ini", QSettings::IniFormat))
{
}

void DefaultRequestHandler::service(HttpRequest &request, HttpResponse &response)
{
    QByteArray path = request.getPath().mid(1);

    if (path == "")
    {
        response.redirect("index");
    }

    if (path.startsWith("static/"))
    {
        path = path.mid(7);
        // todo
        return;
    }

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
