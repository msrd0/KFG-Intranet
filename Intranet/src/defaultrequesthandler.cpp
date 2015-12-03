#include "defaultrequesthandler.h"

DefaultRequestHandler::DefaultRequestHandler(QObject *parent)
    : HttpRequestHandler(parent)
{
}

void DefaultRequestHandler::service(HttpRequest &request, HttpResponse &response)
{

}
