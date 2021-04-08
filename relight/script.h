#ifndef SCRIPT_H
#define SCRIPT_H

#include "task.h"

class QTemporaryFile;

class Script: public Task {
	Q_OBJECT
public:
	QString script_dir;
	QString script_filename;

	QList<QTemporaryFile *> tmp_files;

	virtual ~Script();

	//convenience class to convert  parameters into argv.
	QStringList arguments();
	virtual void run() override;
	virtual void pause() override;
	virtual void resume() override;
	virtual void stop() override;
};

/* pause in linux!
 *
 * #include <signal.h>
...
kill(process->pid(), SIGSTOP); // suspend
kill(process->pid(), SIGCONT); // resume */


#endif // SCRIPT_H
