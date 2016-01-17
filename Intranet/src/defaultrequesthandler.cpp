#include "defaultrequesthandler.h"

#ifdef Q_OS_UNIX
#  include <unistd.h>
#else
#  warning Some functions might not be available on non-unix platforms
#endif

#include <QBuffer>
#include <QCryptographicHash>
#include <QDate>
#include <QImage>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

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

DefaultRequestHandler::DefaultRequestHandler(const QDir &dataDir, const QByteArray &prep, QObject *parent)
	: HttpRequestHandler(parent)
	, prepend(prep.endsWith('/') ? prep : prep+"/")
	, sessionStore(new QSettings(":/sessionstore.ini", QSettings::IniFormat))
	, staticFiles(new QSettings(":/static.ini", QSettings::IniFormat))
	, templates(new QSettings(":/html.ini", QSettings::IniFormat))
{
	db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
	db->setDatabaseName(dataDir.absoluteFilePath("db-v1")); // may increase db version in future
	if (!db->open())
		db = 0;
	db->exec("PRAGMA foreign_keys = ON;");
	CREATE_TABLE("admins",
				 "password TEXT NOT NULL");
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
				 "text TEXT NOT NULL,"
				 "edited DATETIME NOT NULL");
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
	qDebug() << request.getIP() << request.getMethod() << path << request.getVersion();
	response.setHeader("Server", QByteArray("KFG-Intranet (QtWebApp ") + getQtWebAppLibVersion() + ")");
	
	if (!db)
	{
		response.setStatus(500, "Internal Server Error");
		Template base = templates.getTemplate("base");
		QByteArray err = "ERROR: Failed to load the database!!!";
		if (base.isEmpty())
			response.write("<html><body><p>" + err + "</p></body></html>", true);
		else
		{
			base.setVariable("body", err);
			response.write(base.toUtf8(), true);
		}
		return;
	}
	
	if (path == "")
	{
		response.redirect(prepend + "index");
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
	
	if (path == "logout")
	{
		session.set("loggedin", false);
		response.redirect(prepend + "index");
		return;
	}
	
	if (path == "edititem")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect("/administration");
			return;
		}
		
		if (request.getParameter("item").isEmpty())
		{
			response.write("<html><body>"
						   "<p>ERROR: No item specified. This probably happens because you were trying to add an "
						   "item by editing a non-existing item. <a href=\"" + prepend + "administration\">back</a></p>"
						   "</body></html>", true);
			return;
		}
		
		QByteArray action = request.getParameter("action");
		if (action == "delete")
		{
			QSqlQuery q(*db);
			if (!q.exec("DELETE FROM items WHERE item_name='" + request.getParameter("item").replace("'", "''") + "';"))
			{
				qCritical() << q.lastError();
				response.setHeader("Content-Type", "text/plain; charset=utf-8");
				response.setStatus(500, "Internal Server Error");
				response.write(q.lastError().text().toUtf8(), true);
				return;
			}
#ifdef QT_DEBUG
			qDebug() << q.lastQuery();
#endif
			response.redirect(prepend + "administration");
			return;
		}
		
		QTemporaryFile *img = request.getUploadedFile("img");
		bool changeimg = img && !request.getParameter("img").isEmpty();
		QImage image;
		QByteArray imagedata;
		if (changeimg)
		{
			if (!image.load(img, 0))
				changeimg = false;
			else
			{
				image = image.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				QBuffer buf(&imagedata);
				buf.open(QIODevice::WriteOnly);
				image.save(&buf, "PNG");
			}
		}
		QSqlQuery q(*db);
		if (!q.exec("UPDATE items SET "
					"item_name='" + request.getParameter("name").replace("'", "''") + "'," +
					"item_link='" + request.getParameter("link").replace("'", "''") + "'" +
					(changeimg ? ",item_img='" + imagedata.toBase64(QByteArray::KeepTrailingEquals) + "'" : "") +
					" WHERE item_name='" + request.getParameter("item").replace("'", "''") + "';"))
		{
			qDebug() << q.lastQuery();
			qCritical() << q.lastError();
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write(q.lastError().text().toUtf8(), true);
			return;
		}
		response.redirect(prepend + "administration");
		return;
	}
	
	if (path == "additem")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		QByteArray row = request.getParameter("row");
		if (row == "new")
		{
			QSqlQuery q(*db);
			if (!q.exec("INSERT INTO rows (row_name) VALUES ('" + request.getParameter("rowname").replace("'", "''") + "');"))
			{
				qDebug() << q.lastQuery();
				qCritical() << q.lastError();
				response.setHeader("Content-Type", "text/plain; charset=utf-8");
				response.setStatus(500, "Internal Server Error");
				response.write(q.lastError().text().toUtf8(), true);
				return;
			}
	#ifdef QT_DEBUG
			qDebug() << q.lastQuery();
	#endif
			if (!q.exec("SELECT id FROM rows WHERE row_name='" + request.getParameter("rowname").replace("'", "''") + "';") || !q.first())
			{
				qDebug() << q.lastQuery();
				qCritical() << q.lastError();
				response.setHeader("Content-Type", "text/plain; charset=utf-8");
				response.setStatus(500, "Internal Server Error");
				response.write(q.lastError().text().toUtf8(), true);
				return;
			}
	#ifdef QT_DEBUG
			qDebug() << q.lastQuery();
	#endif
			row = q.value("id").toByteArray();
		}
		
		QSqlQuery q(*db);
		if (!q.exec("INSERT INTO items (row, item_name, item_link) VALUES (" + row + ", '" + request.getParameter("name").replace("'", "''") + "', '" + request.getParameter("link").replace("'", "''") + "');"))
		{
			qDebug() << q.lastQuery();
			qCritical() << q.lastError();
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write(q.lastError().text().toUtf8(), true);
			return;
		}
#ifdef QT_DEBUG
		qDebug() << q.lastQuery();
#endif
		response.redirect(prepend + "administration");
		return;
	}
	
	if (path == "addnews")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		QSqlQuery q(*db);
		if (!q.exec("INSERT INTO news (text, edited) VALUES ('" + request.getParameter("text").replace("'", "''") + "', " + QString::number(QDateTime::currentMSecsSinceEpoch()) + ");"))
		{
			qDebug() << q.lastQuery();
			qCritical() << q.lastError();
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write(q.lastError().text().toUtf8(), true);
			return;
		}
#ifdef QT_DEBUG
		qDebug() << q.lastQuery();
#endif
		response.redirect(request.getParameter("redir"));
		return;
	}
	
	if (path.startsWith("itemimage/"))
	{
		QSqlQuery q(*db);
		if (!q.exec("SELECT item_img FROM items WHERE item_name='" + path.mid(10).replace("'", "''") + "';") || !q.first())
		{
			qDebug() << q.lastQuery();
			qCritical() << q.lastError();
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write(q.lastError().text().toUtf8(), true);
			return;
		}
		response.setHeader("Content-Type", "image/png");
#ifndef QT_DEBUG
		response.setHeader("Cache-Control", "max-age=600");
#endif
		response.write(QByteArray::fromBase64(q.value(0).toByteArray()), true);
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
				base.setVariable("body", "<p>A new password has been printed out to stdout of the server. <a href=\"" + prepend + "administration\">back</a></p>");
			}
		}
	}
	else if (path == "changepw")
	{
		if (QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
			base.setVariable("body", "<p>You might only change a password with a POST request. <a href=\"" + prepend + "administration\">back</a></p>"
							 "<p>Your request type: <code>" + request.getMethod() + "</code></p>");
		else if (request.getParameter("newpw").isEmpty())
			base.setVariable("body", "<p>Your password might not be empty. <a href=\"" + prepend + "administration\">back</a></p>");
		else if (request.getParameter("newpw") != request.getParameter("pwwdh"))
			base.setVariable("body", "<p>The passwords you entered didn't match! <a href=\"" + prepend + "administration\">back</a></p>");
		else
		{
			QSqlQuery q = db->exec("SELECT rowid, password FROM admins;");
#ifdef QT_DEBUG
			qDebug() << q.lastQuery();
#endif
			if (q.lastError().isValid())
			{
				qCritical() << q.lastError();
				base.setVariable("body", "<p>" + q.lastError().text().toUtf8() + "</p>");
			}
			else
			{
				QByteArray pw = request.getParameter("oldpw");
				int rowid = -1;
				while (q.next())
				{
					if (passwordMatch(pw, q.value("password").toByteArray()))
					{
						rowid = q.value("rowid").toInt();
						break;
					}
				}
				if (rowid == -1)
					base.setVariable("body", "<p>The entered passwort did not match with the one you used to log in. <a href=\"" + prepend + "administration\">back</a></p>");
				else
				{
					if (exec("UPDATE admins SET password='" + password(request.getParameter("newpw")) + "' WHERE rowid='" + QString::number(rowid) + "';"))
					{
						loggedin = false;
						session.set("loggedin", false);
						base.setVariable("body", "<p>Your password has been successfully updated. <a href=\"" + prepend + "administration\">Log In</a></p>");
					}
					else
						base.setVariable("body", "<p>Failed to update password. <a href=\"" + prepend + "administration\">back</a></p>");
				}
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
			// redirect parameters
			for (auto key : request.getParameterMap().keys())
				t.setVariable("param-" + key, request.getParameter(key));
			t.setCondition("wrongpw", request.getParameter("wrongpw") == "true");
			
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
				QList<QPair<QPair<int, QString>, QList<Item> > > l;
				while (items.next())
				{
					int row = items.value("row").toInt();
					while (l.size() <= row)
						l.append(qMakePair(QPair<int, QString>(), QList<Item>()));
					l[row].first = qMakePair(row, items.value("row_name").toString());
					l[row].second << Item{items.value("item_name").toString(), prepend + "itemimage/" + QUrl::toPercentEncoding(items.value("item_name").toByteArray()), items.value("item_link").toString()};
				}
				
				for (int i = 0; i < l.size();)
				{
					if (l[i].second.isEmpty())
						l.removeAt(i);
					else
						i++;
				}
				
				t.loop("gridrow", l.size());
				for (int i = 0; i < l.size(); i++)
				{
					auto &a = l[i];
					t.setVariable("gridrow" + QString::number(i) + ".name", a.first.second);
					t.setVariable("gridrow" + QString::number(i) + ".index", QString::number(i));
					t.setVariable("gridrow" + QString::number(i) + ".id", QString::number(a.first.first));
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
	
	QSqlQuery news = db->exec("SELECT * FROM news ORDER BY edited DESC LIMIT 5;");
#ifdef QT_DEBUG
	qDebug() << news.lastQuery();
#endif
	if (news.lastError().isValid())
	{
		qCritical() << news.lastError();
		base.loop("news", 1);
		base.setVariable("news0.text", news.lastError().text());
		base.setVariable("news0.edited", "SERVER ERROR");
	}
	else
	{
		int size = news.last() ? news.at()+1 : 0;
		if (!news.first())
			size = 0;
		base.loop("news", size);
		for (int i = 0; (i==0 || news.next()) && i<size; i++)
		{
			qDebug() << i;
			base.setVariable("news" + QString::number(i) + ".text", news.value("text").toString());
			base.setVariable("news" + QString::number(i) + ".edited", QDateTime::fromMSecsSinceEpoch(news.value("edited").toLongLong()).date().toString("ddd, dd. MMMM yyyy"));
		}
	}
	
	base.setVariable("name", path);
	base.setVariable("prepend", prepend);
	base.setCondition("loggedin", loggedin);
	response.write(base.toUtf8(), true);
}
