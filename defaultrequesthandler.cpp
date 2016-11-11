#include "defaultrequesthandler.h"
#include "db_intranet.h"

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
#include <QUrl>

using namespace qsl;

static QByteArray salt (uint length)
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

DefaultRequestHandler::DefaultRequestHandler(const QDir &sharedDir, const QDir &dataDir, const QByteArray &prep, QObject *parent)
	: HttpRequestHandler(parent)
	, sharedDir(sharedDir)
	, prepend(prep.endsWith('/') ? prep : prep+"/")
	, sessionStore(new QSettings(sharedDir.absoluteFilePath("conf/sessionstore.ini"), QSettings::IniFormat))
	, staticFiles(new QSettings(sharedDir.absoluteFilePath("conf/static.ini"), QSettings::IniFormat))
	, templates(new QSettings(sharedDir.absoluteFilePath("conf/html.ini"), QSettings::IniFormat))
	, d("sqlite")
	, helpmdfile(dataDir.absoluteFilePath("help.md"))
{
	int migrate = 3;
	QString dbFile = dataDir.absoluteFilePath("intranet.db");
	
	if (!QFileInfo(dbFile).exists())
	{
		qDebug() << "database does not exist";
		for (int i = migrate - 1; i > 0; i--)
		{
			QString f = dataDir.absoluteFilePath("db-v" + QString::number(i));
			qDebug() << "checking whether" << f << "exists";
			if (QFileInfo(f).exists())
			{
				qDebug() << "copying" << f << "to" << dbFile;
				QFile::copy(f, dbFile);
				migrate = i;
				qDebug() << "going to migrate database from version" << migrate;
				break;
			}
		}
	}
	
	d.setName(dbFile);
	if (!d.connect())
	{
		qCritical() << "Failed to connect to database; will exit";
		exit(1);
	}
	
	helpmdmutex.lock();
	if (helpmdfile.exists())
	{
		helpmd = "### Es wurden noch keine Hilfeseiten erstellt\n";
		helpmdfile.open(QIODevice::WriteOnly);
		helpmdfile.write(helpmd);
	}
	else
	{
		helpmdfile.open(QIODevice::ReadWrite);
		helpmd = helpmdfile.readAll();
	}
	helpmdmutex.unlock();
}

void DefaultRequestHandler::service(HttpRequest &request, HttpResponse &response)
{
	HttpSession session = sessionStore.getSession(request, response);
	bool loggedin = session.get("loggedin").toBool();
	
	QByteArray path = request.getPath().mid(1);
	qDebug() << request.getPeerAddress() << request.getMethod() << path << request.getVersion();
	response.setHeader("Server", QByteArray("KFG-Intranet (QtWebApp ") + getQtWebAppLibVersion() + ")");
	
	if (path == "")
	{
		response.redirect(prepend + "index");
		return;
	}
	
	if (path == "help/content.js")
	{
		QByteArray varname = request.getParameter("var");
		if (varname.isEmpty())
			varname = "helpcontent";
		
		response.setHeader("Content-Type", "text/javascript");
		helpmdmutex.lock();
		response.write("var " + varname + " = '" + helpmd.replace('\n', "\\n").replace('\'', "\\'") + "';\n", true);
		helpmdmutex.unlock();
		return;
	}
	
	if (path == "login")
	{
		auto q = d.adminstrators().query();
		QByteArray pw = request.getParameter("pw");
		bool success = false;
		for (auto p : q)
		{
			if (p.password() == pw)
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
	
	if (path == "edithelp")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		qDebug() << request.getParameterMap();
		
		helpmdmutex.lock();
		helpmd = request.getParameter("content").replace('\r', "");
		helpmdfile.seek(0);
		helpmdfile.resize(helpmd.size());
		helpmdfile.write(helpmd);
		helpmdmutex.unlock();
		
		response.redirect(prepend + "help");
		return;
	}
	
	if (path == "editrowtitle")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		auto q = d.rows().filter("id" EQ request.getParameter("id")).query();
		if (q.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find the row with the given id", true);
			return;
		}
		
		if (!q[0].setRow_name(request.getParameter("title")))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to update the row title", true);
			return;
		}
		
		response.redirect(prepend + "administration");
		return;
	}
	
	if (path == "edititem")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
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
			if (!d.items().filter("item_name" EQ request.getParameter("item")).remove())
			{
				response.setHeader("Content-Type", "text/plain; charset=utf-8");
				response.setStatus(500, "Internal Server Error");
				response.write("ERROR: Unable to delete the requested item", true);
				return;
			}
			
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
		
		auto q = d.items().filter("item_name" EQ request.getParameter("item")).query();
		if (q.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find item with the given name", true);
			return;
		}
		
		if (!q[0].setItem_name(request.getParameter("name"))
				|| !q[0].setItem_link(request.getParameter("link"))
				|| (changeimg && !q[0].setItem_img(imagedata.toBase64(QByteArray::KeepTrailingEquals))))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to update the requested item", true);
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
		
		if (request.getParameter("name").isEmpty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("You have attempted to create an item without a name. This is forbidden.", true);
			return;
		}
		
		if (request.getParameter("row") == "new")
		{
			if (!d.rows().insert({request.getParameter("rowname")}))
			{
				response.setHeader("Content-Type", "text/plain; charset=utf-8");
				response.setStatus(500, "Internal Server Error");
				response.write("ERROR: Unable to create the requested row", true);
				return;
			}
		}
		
		auto row = d.rows().filter("row_name" EQ request.getParameter("row")).query();
		if (row.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find the row", true);
			return;
		}
		
		QFile defImg(sharedDir.absoluteFilePath("img/noimg.png"));
		defImg.open(QIODevice::ReadOnly);
		if (!d.items().insert({row[0], request.getParameter("name"), defImg.readAll().toBase64(QByteArray::KeepTrailingEquals), request.getParameter("link")}))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to insert item", true);
			defImg.close();
			return;
		}
		
		response.redirect(prepend + "administration");
		defImg.close();
		return;
	}
	
	if (path == "addnews")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		if (!d.news().insert({request.getParameter("text"), QDateTime::currentDateTime()}))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to insert news", true);
			return;
		}
		
		response.redirect(request.getParameter("redir"));
		return;
	}
	
	if (path == "editnews")
	{
		if (!loggedin || QString::compare(request.getMethod(), "post", Qt::CaseInsensitive) != 0)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		auto q = d.news().filter("news_id" EQ request.getParameter("id")).query();
		if (q.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find news with the given id", true);
			return;
		}
		
		if (!q[0].setText(request.getParameter("text"))
				|| !q[0].setEdited(QDateTime::currentDateTime()))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to update news", true);
			return;
		}
		
		response.redirect(request.getParameter("redir"));
		return;
	}
	
	if (path == "deletenews")
	{
		if (!loggedin)
		{
			response.redirect(prepend + "administration");
			return;
		}
		
		auto q = d.news().filter("news_id" EQ request.getParameter("id")).query();
		if (q.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find news with the given id", true);
			return;
		}
		
		if (!d.news().remove(q[0]))
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(500, "Internal Server Error");
			response.write("ERROR: Unable to delete news", true);
			return;
		}
		
		// no redir - called via jquery
		return;
	}
	
	if (path.startsWith("itemimage/"))
	{
		auto q = d.items().filter("item_name" EQ path.mid(10)).query();
		if (q.empty())
		{
			response.setHeader("Content-Type", "text/plain; charset=utf-8");
			response.setStatus(400, "Bad Request");
			response.write("ERROR: Unable to find the requested item", true);
			return;
		}
		
		response.write(QByteArray::fromBase64(q[0].item_img()), true);
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
			base.setVariable("body", "ERROR: The stdout of the server doesn't point to a tty.");
		else
#endif
		{
			QByteArray pw = salt(6).toBase64(QByteArray::OmitTrailingEquals);
			if (!d.adminstrators().insert({pw}))
				base.setVariable("body", "Failed to create a new password. Check the server log for details.");
			else
			{
				qDebug() << "NEW PASSWORD GENERATED:" << pw << "(requested from " << request.getPeerAddress() << ")";
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
			auto q = d.adminstrators().query();
			int in = -1;
			
			QByteArray pw = request.getParameter("oldpw");
			for (int i = 0; i < q.size(); i++)
			{
				if (q[i].password() == pw)
				{
					in = i;
					break;
				}
			}
			if (in == -1)
				base.setVariable("body", "<p>The entered passwort did not match with the one you used to log in. <a href=\"" + prepend + "administration\">back</a></p>");
			else
			{
				if (q[in].setPassword(request.getParameter("newpw")))
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
			
			auto items = d.items().query();
			
			QList<QPair<QPair<int, QString>, QList<Item> > > l;
			for (auto item : items)
			{
				int row = item.row().id();
				while (l.size() <= row)
					l.append(qMakePair(QPair<int, QString>(), QList<Item>()));
				l[row].first = qMakePair(row, item.row().row_name());
				l[row].second << Item{item.item_name(), prepend + "itemimage/" + QUrl::toPercentEncoding(item.item_name()), item.item_link()};
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
	
	auto news = d.news().desc().limit(5).query();
	base.loop("news", news.size());
	for (int i = 0; i < news.size(); i++)
	{
		base.setVariable("news" + QString::number(i) + ".id", QString::number(news[i].news_id()));
		base.setVariable("news" + QString::number(i) + ".text", news[i].text());
		base.setVariable("news" + QString::number(i) + ".edited", news[i].edited().date().toString("ddd, dd. MMMM yyyy"));
	}
	
	base.setVariable("version", INTRANET_VERSION_STRING);
	base.setVariable("name", path);
	base.setVariable("prepend", prepend);
	base.setCondition("loggedin", loggedin);
	response.write(base.toUtf8(), true);
}
