#include "httpserver.h"
#include "httplib.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFile>

#include <iostream>
using namespace std;
using namespace httplib;

HttpServer::HttpServer() {
	server = new Server;

	addFile("/",                ":/demo/index.html",      "text/html");
	addFile("/index.html",      ":/demo/index.html",      "text/html");
	addFile("/skin.css",        ":/demo/skin.css",        "text/css");
	addFile("/skin.svg",        ":/demo/skin.svg",        "image/svg+xml");
    addFile("/openlime.min.js", ":/demo/openlime.min.js", "text/javascript");
    addFile("/openlime.js", ":/demo/openlime.js", "text/javascript");
}
HttpServer::~HttpServer() {
	if(server)
		stop();
	delete server;
}

void HttpServer::addFile(const std::string &url, const std::string &path, const  std::string &mime) {
	server->Get(url.c_str(), [path, mime](const Request& /*req*/, Response& res) {
		QFile file(path.c_str());
		file.open(QFile::ReadOnly);
		res.set_content(file.readAll().data(), mime.c_str());
	});
}

void HttpServer::start(QString folder) {
	stop();

	server->remove_mount_point("/");
	auto ret = server->set_mount_point("/", folder.toStdString().c_str());
	if(!ret)
		throw QString("Could not mount folder " + folder + " for http server.");



	t = std::thread([this](){
		this->server->listen("0.0.0.0", port);
	});
}

void HttpServer::stop() {
	if(!server)
		return;
	server->stop();
	try {
		if(t.joinable())
			t.join();
	} catch(std::system_error &error) {
		cout << "What? " << error.what() << endl;
	}
}

void HttpServer::show() {
	QDesktopServices::openUrl(QUrl(QString("http://localhost:%1").arg(port)));
}
