// Diagnostics.h

#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <atomic>
#include <cstdio>

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/statvfs.h>

#include "../Log.h"
#include "../ShadowXMessageSender.h"
#include "../TextColor.h"
#include "../ThreadRunnerBase.h"
#include "../SharedMemory.h"

using namespace std;

class Diagnostics : public ThreadRunnerBase
{
public:
	Diagnostics();
	bool Init(int outbound_fd);
private:
	ShadowXMessageSender m_protobufSender;
	// This defines the interval in between diagnostic messages
	const int DiagnosticsInterval = 15000;  // milliseconds (15 secs)
	const int DiagnosticsIntervalShort = 5000;  // 5 secs
	const char *FilesysPath = "/home/root";
	void _thread_proc(void);
};

#endif // __DIAGNOSTICS_H__
