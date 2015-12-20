#include "defaultrequesthandler.h"

#ifdef Q_OS_UNIX
#  include <unistd.h>
#else
#  warning Some functions might not be available on non-unix platforms
#endif

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

#ifndef PASSWORD_SALT_LENGTH
#  define PASSWORD_SALT_LENGTH 32
#endif
#ifndef PASSWORD_REPEATS
#  define PASSWORD_REPEATS 1000
#endif

QByteArray salt (uint length)
{
	QByteArray bytes;
#ifdef Q_OS_UNIX
	QFile urand("/dev/urandom");
	if (urand.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
	{
		while (bytes.length() < length)
		{
			bytes.append(urand.read(length - bytes.length()));
		}
		bytes = bytes.mid(0, length);
	}
	else
#endif
	{
		for (uint i = 0; i < length; i++)
			bytes.append((unsigned char)(qrand() % 256));
	}
	return bytes;
}

QByteArray password (const QByteArray &in)
{
	QByteArray pw = "$" + QByteArray::number(PASSWORD_REPEATS) + "$";
	QByteArray s = salt(PASSWORD_SALT_LENGTH);
	pw.append(s.toBase64()).append("$");
	QByteArray hash = s + in;
	for (uint i = 0; i < PASSWORD_REPEATS; i++)
		hash = QCryptographicHash::hash(s + hash, QCryptographicHash::Sha256);
	pw.append(hash.toBase64()).append("$");
	return pw;
}
bool passwordMatch(const QByteArray &pw, const QByteArray &db)
{
	static QRegularExpression regex("^\\$(?P<repeats>\\d+)\\$(?P<salt>[^\\$]+)\\$(?P<hash>[^\\$]+)\\$$");
	QRegularExpressionMatch match = regex.match(db);
	if (!match.hasMatch())
		return false;
	uint db_repeats = match.captured("repeats").toUInt();
	QByteArray db_salt = QByteArray::fromBase64(match.captured("salt").toLatin1());
	QByteArray db_hash = QByteArray::fromBase64(match.captured("hash").toLatin1());
	QByteArray hash = db_salt + pw;
	for (uint i = 0; i < db_repeats; i++)
		hash = QCryptographicHash::hash(db_salt + hash, QCryptographicHash::Sha256);
	return (hash == db_hash);
}


/// macro to execute the sql to create a table
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
	, sessionStore(new QSettings)
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
				 "item_name TEXT NOT NULL UNIQUE,"
				 "item_img TEXT,"
				 "item_link TEXT NOT NULL,"
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
	HttpSession session = sessionStore.getSession(request, response);
	bool loggedin = session.get("loggedin").toBool();
	
	QByteArray path = request.getPath().mid(1);
	response.setHeader("Server", QByteArray("KFG-Intranet (QtWebApp ") + getQtWebAppLibVersion() + ")");
	
	if (path == "")
	{
		response.redirect("index");
		return;
	}
	
	if (path == "login")
	{
		QSqlQuery q = db->exec("SELECT password FROM admins;");
#ifdef QT_DEBUG
		qDebug() << q.lastQuery();
#endif
		if (q.lastError().isValid())
		{
			qCritical() << q.lastError();
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write(q.lastError().text().toUtf8(), true);
			return;
		}
		QByteArray pw = request.getParameter("pw");
		bool success = false;
		while (q.next())
		{
			if (passwordMatch(pw, q.value("password").toByteArray()))
			{
				success = true;
				break;
			}
		}
		if (success)
		{
			session.set("loggedin", true);
			response.redirect(request.getParameter("redir"));
			return;
		}
		session.set("loggedin", false);
		response.redirect(request.getParameter("redir") + "?wrongpw=true");
		return;
	}
	
	if (path.startsWith("static/"))
	{
		staticFiles.service(request, response);
		return;
	}
	
	response.setHeader("Content-Type", "text/html; charset=utf-8");
	Template base = templates.getTemplate("base");
	if (path == "base")
	{
		response.setStatus(403, "Forbidden");
		base.setVariable("body", "403 Forbidden");
	}
	else if (path == "newpw")
	{
#ifdef Q_OS_UNIX
		if (!isatty(STDOUT_FILENO))
		{
			base.setVariable("body", "ERROR: The stdout of the server doesn't point to a tty.");
		}
		else
#endif
		{
			QByteArray pw = salt(6).toBase64(QByteArray::OmitTrailingEquals);
			if (!exec("INSERT INTO admins (password) VALUES ('" + password(pw) + "');"))
			{
				base.setVariable("body", "Failed to create a new password. Check the server log for details.");
			}
			else
			{
				qDebug() << "NEW PASSWORD GENERATED:" << pw << "(requested from " << request.getIP() << ")";
				base.setVariable("body", "A new password has been printed out to stdout of the server.");
			}
		}
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
			QSqlQuery items = db->exec("SELECT * FROM items INNER JOIN rows ON row=rows.id ORDER BY row;");
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
				QList<QPair<QString, QList<Item> > > l;
				while (items.next())
				{
					int row = items.value("row").toInt();
					while (l.size() <= row)
						l.append(qMakePair(QString(), QList<Item>()));
					l[row].first = items.value("row_name").toString();
					l[row].second << Item{items.value("item_name").toString(), items.value("item_img").toString(), items.value("item_link").toString()};
				}
				t.loop("gridrow", l.size());
				for (int i = 0; i < l.size(); i++)
				{
					auto &a = l[i];
					t.setVariable("gridrow" + QString::number(i) + ".name", a.first);
					int j;
					for (j = 0; j < a.second.size(); j++)
					{
						t.setVariable("gridrow" + QString::number(i) + ".col"  + QString::number(j), a.second[j].name);
						t.setVariable("gridrow" + QString::number(i) + ".img"  + QString::number(j), a.second[j].img.isEmpty() ? "none" : "url(" + a.second[j].img + ")");
						t.setVariable("gridrow" + QString::number(i) + ".link" + QString::number(j), a.second[j].link);
					}
					for (; j < 4; j++)
					{
						t.setVariable("gridrow" + QString::number(i) + ".col"  + QString::number(j), QString());
						t.setVariable("gridrow" + QString::number(i) + ".img"  + QString::number(j), "none");
						t.setVariable("gridrow" + QString::number(i) + ".link" + QString::number(j), "#");
					}
				}
				
				base.setVariable("body", t);
			}
		}
	}
	base.setVariable("name", path);
	base.setCondition("loggedin", loggedin);
	base.setCondition("wrongpw", request.getParameter("wrongpw") == "true");
	response.write(base.toUtf8(), true);
}
