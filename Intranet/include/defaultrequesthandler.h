#ifndef DEFAULTREQUESTHANDLER_H
#define DEFAULTREQUESTHANDLER_H

#include <httprequesthandler.h>
#include <staticfilecontroller.h>
#include <templatecache.h>

class DefaultRequestHandler : public HttpRequestHandler
{
    Q_OBJECT

public:
    DefaultRequestHandler(QObject *parent = 0);

    void service(HttpRequest& request, HttpResponse& response);

private:
    StaticFileController staticFiles;
    TemplateCache templates;

};

#endif // DEFAULTREQUESTHANDLER_H
