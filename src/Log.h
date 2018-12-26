// Log.h
// This and Log.cpp are used by ShadowX, headlessController
// and possibly Sqlite3Server and other ShadowX related applications.
// For this reason define the LOGFILE_NAME=xxx in the MAKEFILE
// for each individual project. (USED TO BE #define'd in here).

// NB: For GetStackTrace() to work properly, *MUST*
// include '-rdynamic' in CFLAGS in Makefile and then this
// will showfunc name, else just shows 'lanshark_2..._bin() [0x(ofst)]

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <mutex>

#include <cstdio>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <time.h>

#include "TextColor.h"

using namespace std;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__) ": "

#ifndef LOGFILE_NAME
// This USED TO BE defined right here, moved into makefile
// so other apps can use this same module.
// #error "Please add define for LOGFILE_NAME in your makefile,"
// #error "See shadow makefile for example (too many quotes needed here)"
// #error "LOGFILENAME=\"/home/root/shadow.log\""
//
// This is for the original GUMSTIX platform:
//#define LOGFILE_NAME "/home/root/shadowx.log"

// This is the NanoPi NEOPLUS platform
// (allows Android app to access the log file):
#define LOGFILE_NAME "/home/pi/i2c/i2c.log"

#endif

#define MAX_LOG_FILE_SIZE 65535
#define KEEP_LAST_LOG_SIZE 32767

enum LogInfoColors
{
	LogInfoYellow = 0,
	LogInfoGreen,
	LogInfoBlue,
	LogInfoCyan,
	LogInfoMagenta,
	LogInfoWhite
};

// ALL Log instances have access to the list of "Pending Messages
// waiting to be written. Please put a mutex around access to this.
class PendingMessages
{
private:
	PendingMessages() { }
	PendingMessages(PendingMessages const& copy);  // Not allowed
	PendingMessages& operator=(PendingMessages const& copy);  // Not allowed
	vector<string>m_pendingMessages;
	static PendingMessages *m_instance;
public:
	vector<string>& GetPending(void)
	{
		return m_pendingMessages;
	}

	static PendingMessages *GetInstance(void)
	{
		if (m_instance == nullptr)
		{
			m_instance = new PendingMessages();
		}
		return m_instance;
	}
};

class Log
{
public:
	Log(const char *owner);
	Log();
	void SetLogName(const char *owner);
	void LogErr(const char *at, int errnum);
	void LogErr(const char *at, const char *msg, int errnum);
	void LogErr(const char *at, const char *msg);
	void LogErr(const char *at, const string& msg);
	void LogErr(const char *at, const stringstream& msg);
	void LogInfo(const char *msg);
	void LogInfo(const string& msg);
	void LogInfo(const stringstream& msg);
	void LogHeader(const char *header_msg, LogInfoColors blockColor);
	void LogEndHeader(void);
	const string GetStackTrace(const char *who);
protected:
	string m_logOwnerName;
private:
	int m_sequenceNumber = 0;
	LogInfoColors m_infoColor = LogInfoColors::LogInfoYellow;
	bool m_isInited = false;
	struct flock m_flock;
	void _initFlock(void);
	int _open();
	bool _close(int fd);
	void FillTime(char *buf);
	void _logIt(const char *msg, const char *at, bool isAnError);
	mutex m_mutex;
	static const constexpr char* const m_infoColors[] =
	{
		TEXT_YELLOW,
		TEXT_GREEN,
		TEXT_BLUE,
		TEXT_CYAN,
		TEXT_MAGENTA,
		TEXT_WHITE
	};
};

#endif  // LOG_H_
