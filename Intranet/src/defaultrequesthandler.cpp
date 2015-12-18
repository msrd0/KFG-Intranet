#include "defaultrequesthandler.h"

#include <QSqlError>
#include <QSqlQuery>

#define CREATE_TABLE(name, sql) \
	if (!db) \
	{ \
		qDebug() << "Error in " __FILE__ ":" << __LINE__ << ": no db available"; \
	} \
	else if (!exec("CREATE TABLE IF NOT EXISTS '" name "' (" sql ");")) \
	{ \
		qDebug() << "Error was in " __FILE__ ":" << __LINE__; \
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
	CREATE_TABLE("admins",
				 "password TEXT");
	CREATE_TABLE("rows",
				 "id INTEGER PRIMARY KEY UNIQUE,"
				 "row_name TEXT UNIQUE");
	CREATE_TABLE("items",
				 "row INTEGER,"
				 "item_name TEXT UNIQUE,"
				 "item_img TEXT,"
				 "FOREIGN KEY(row) REFERENCES rows(id)");
	CREATE_TABLE("news",
				 "text TEXT,"
				 "edited DATETIME");
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
			QSqlQuery items = db->exec("SELECT * FROM items INNER JOIN rows ORDER BY row;");
#ifdef QT_DEBUG
			qDebug() << items.lastQuery();
#endif
			if (items.lastError().isValid())
			{
				qCritical() << items.lastError();
				base.setVariable("body", items.lastError().text());
			}
			else
			{
				QList<QPair<QString, QList<QPair<QString, QString> > > > l;
				while (items.next())
				{
					int row = items.value("row").toInt();
					while (l.size() <= row)
						l.append(qMakePair(QString(), QList<QPair<QString, QString> >()));
					l[row].first = items.value("row_name").toString();
					l[row].second << qMakePair(items.value("item_name").toString(), items.value("item_img").toString());
				}
				t.loop("gridrow", l.size());
				for (auto a : l)
				{
					t.setVariable("gridrow0.name", a.first);
					int i;
					for (i = 0; i < a.second.size(); i++)
					{
						t.setVariable("gridrow0.col" + QString::number(i), a.second[i].first);
						t.setVariable("gridrow0.img" + QString::number(i), a.second[i].second.isEmpty() ? "none" : "url(" + a.second[i].second + ")");
					}
					for (; i < 4; i++)
					{
						t.setVariable("gridrow0.col" + QString::number(i), QString());
						t.setVariable("gridrow0.img" + QString::number(i), "none");
					}
				}
				
				base.setVariable("body", t);
			}
		}
	}
	response.write(base.toUtf8(), true);
}
