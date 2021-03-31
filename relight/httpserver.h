#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QString>
#include <thread>

namespace httplib {
	class Server;
}


class HttpServer {
public:
	HttpServer();
	~HttpServer();
public:
	int port = 61007;
	void start(QString folder);
	void stop();
	void show();
	void addFile(const std::string &url, const std::string &path, const  std::string &mime);
private:
	httplib::Server *server = nullptr;
	std::thread t;
};

#endif // HTTPSERVER_H
