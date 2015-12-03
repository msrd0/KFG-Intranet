#ifndef DEFAULTREQUESTHANDLER_H
#define DEFAULTREQUESTHANDLER_H

#include <httprequesthandler.h>

class DefaultRequestHandler : public HttpRequestHandler
{
    Q_OBJECT

public:
    DefaultRequestHandler(QObject *parent = 0);

    void service(HttpRequest& request, HttpResponse& response);

};

#endif // DEFAULTREQUESTHANDLER_H
