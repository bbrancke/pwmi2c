// Log.cpp
// Logging for Sqlite3Server.
#include "Log.h"

PendingMessages *PendingMessages::m_instance = nullptr;
constexpr const char* const Log::m_infoColors[];

Log::Log(const char *owner)
{
	SetLogName(owner);
	_initFlock();
}
// This Log ctor is so we can utilize ShadowX components
// that don't use the 'module name'; not sure if its useful or not...
// Later: I think we should always give a name esp for DB connections
// that timeout b/cos another DB conn has done something ....
Log::Log()
{
	_initFlock();
}

void Log::_initFlock(void)
{
	m_flock.l_type = F_WRLCK;
	m_flock.l_whence = SEEK_SET;
	m_flock.l_start = 0;
	m_flock.l_len = 0;  // 0 == "Lock to end of file"
	m_flock.l_pid = getpid();
	m_isInited = true;
}

void Log::SetLogName(const char *owner)
{
	m_logOwnerName = owner;
	m_logOwnerName += ": ";
}

void Log::FillTime(char *buf)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	struct tm *tim = gmtime(&ts.tv_sec);
	// 1,000,000 us = 1 sec. 1,000,000 ns = 0.001 seconds.
	ts.tv_nsec /= 1000000;  // 1,000,000 ns = 1 ms. tv_ns is now 0-9999.
	sprintf(buf, "%02d/%02d/%02d %02d:%02d:%02d.%04ld: ",
		(tim->tm_mon + 1), tim->tm_mday, (tim->tm_year % 100),
		tim->tm_hour, tim->tm_min, tim->tm_sec, ts.tv_nsec);
}

void Log::_logIt(const char* msg, const char *at, bool isAnError)
{
	char timeNow[128];
	FillTime(timeNow);  // adds ending space

	stringstream sPid;
	sPid << " PID: " << getpid() << " PPID: " << getppid() << " ";
	// Add sequence # in case log file is locked and this message
	// is put into the PendingMessages container.
	m_sequenceNumber++;
	if (m_sequenceNumber > 999)
	{
		m_sequenceNumber = 1;
	}
	stringstream seq;
	seq << setfill('0') << setw(3) << m_sequenceNumber;
	string savedMsg;
	if (isAnError)
	{
		_RED(msg);
		// Error:
		// <E> [OWNER:] SEQ PID: xxx PPID: xxx DATETIME: [AT]: [msg]\r\n
		//      ++-- m_owner has ": " at the end.
		//  SEQ = 3 digit Sequence Number "001"
		// 'AT' is __FILE__ ":" TOSTRING(__LINE__) ": ", e.g.:
		// "/home/osboxes/yocto2/build/tmp/work/ " +
		//    "cortexta8hf-vfp-neon-poky-linux-gnueabi/shadowx/shadowx-3.0.0-r10/" +
		//    "git/shadowx/shadowx-3.0.0/src/tcp/OneConnectionHandlers/" +
		//    "AndroidCandCProcessor.cpp:523: "
		// This limits how many msgs we have in the log file. Let us reduce this
		// to file name + line number:
		// "AndroidCandCProcessor:523: "
		string newAt(at);
		size_t pos = newAt.find_last_of('/');
		if (pos != string::npos)
		{
			newAt = newAt.substr(pos + 1);
		}
		savedMsg = "<E> ";
		savedMsg += m_logOwnerName;  // "xyz: "
		savedMsg += seq.str();       // "001"
		savedMsg += sPid.str();      // " PID: xxx PPID: xxx "
		savedMsg += timeNow;         // "mm/dd/yy hh:mm:ss.dddd: "
		savedMsg += newAt;
		//savedMsg += at;              // Long string func name + line number
		// Note that the original 'at' already had the ending ':';
		// this has always been like this:
		//savedMsg += ": ";
		// += msg + "\r\n"
	}
	else
	{
		// _YELLOW(msg); Now can change m_infoColor via LogHeader so that
		// (e.g.) "Connecting:" and "DHCP: Obtaining IP address" can be
		// different LogInfo colors.
		std::cout << m_infoColors[m_infoColor] << (msg) << TEXT_NORMAL << std::endl;
		// Info:
		// <I> [OWNER] SEQ PID xxx PPID xxx  DATETIME: [msg]\r\n  (no AT)
		// I == Info, qqq = sequence number.
		savedMsg = "<I> ";
		savedMsg += m_logOwnerName;
		savedMsg += seq.str();
		savedMsg += sPid.str();
		savedMsg += timeNow;
		// += msg + "\r\n"
	}
	savedMsg += msg;
	savedMsg += "\r\n";

	int fd = _open();
	if (fd >= 0)
	{
		// Write any pending error / info messages
		// These are added to m_pendingSaveMessages
		// if _open() fails so we don't block a thread
		// or process if somebody else has an exclusive
		// open on the log file.
		{
			lock_guard<mutex> lock(m_mutex);
			vector<string> pending = PendingMessages::GetInstance()->GetPending();
			for (auto &s : pending)
			{
				const char *p = s.c_str();
				write(fd, p, strlen(p));
			}
			pending.clear();
			// 'lock' goes out of scope and releases m_mutex
		}
		write(fd, savedMsg.c_str(), savedMsg.length());
		_close(fd);
	}
	else
	{
		// Couldn't get exclusive file write access, another
		// thread iis writing. Add to "Pending" and I will clear
		// this out (see above) when I get exclusive access.
		string pend(" * ");  // " * " means logging was deferred
		pend += savedMsg;
		{
			lock_guard<mutex> lock(m_mutex);
			PendingMessages::GetInstance()->GetPending().push_back(pend);
		}
	}
}

void Log::LogErr(const char *at, const char *msg, int errnum)
{
	stringstream s;

	if (strlen(msg) > 0)
	{
		s << msg;
		s << ": ";
	}
	s << strerror(errnum);
	s << " (";
	s << errnum;
	s << ")";
	_logIt(s.str().c_str(), at, true);
}

void Log::LogErr(const char *at, int errnum)
{
	LogErr(at, "", errnum);
}

void Log::LogErr(const char *at, const char *msg)
{
	_logIt(msg, at, true);
}

void Log::LogErr(const char *at, const string& msg)
{
	LogErr(at, msg.c_str());
}

void Log::LogErr(const char *at, const stringstream& msg)
{
	LogErr(at, msg.str().c_str());
}

void Log::LogInfo(const char *msg)
{
	_logIt(msg, nullptr, false);
}

void Log::LogInfo(const string& msg)
{
	LogInfo(msg.c_str());
}

void Log::LogInfo(const stringstream& msg)
{
	LogInfo(msg.str().c_str());
}

void Log::LogHeader(const char* header_msg, LogInfoColors blockColor)
{
	m_infoColor = blockColor;
	LogInfo("--------------------------------------------------------------------------");
	LogInfo(header_msg);
}

void Log::LogEndHeader(void)
{
	LogInfo("--------------------------------------------------------------------------");
	m_infoColor = LogInfoColors::LogInfoYellow;

}


int Log::_open()
{
	off_t fileSize;
	// We READ this file iff log size > 64 KB...
	int fd = open(LOGFILE_NAME, O_RDWR | O_APPEND | O_CREAT,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0)
	{
		cout << "ERROR - LOGGER CANNOT OPEN LOGFILE!" << endl;
		return -1;
	}

	m_flock.l_type = F_WRLCK;  // Exclusive lock
	m_flock.l_pid = getpid();
	// A write lock here prevents others from accessing Logfile
	// until I release it.
	// fcntl(F_SETLKW) BLOCKS until another guy releases their lock
	//    (or EINTR).
	// (vs. F_SETLK which returns immediately with EAGAIN)
	if (fcntl(fd, F_SETLK, &m_flock) == -1)
	{
		// Someone else is writing. I will add to m_pending
		// and return.
		// Later someone will eventually clear out the pendings
		close(fd);
		return -1;
	}
	
	// Keep log file < 64 KB. 
	// If > 64 KB, read the latest 32KB, erase the file
	// and then write the latest 32KB; left with 32 KB log file.
	fileSize = lseek(fd, 0, SEEK_END);
	if (fileSize > MAX_LOG_FILE_SIZE)
	{
		vector<uint8_t>buf(KEEP_LAST_LOG_SIZE);
		lseek(fd, 0 - KEEP_LAST_LOG_SIZE, SEEK_END);
		ssize_t bytes_read = read(fd, buf.data(), KEEP_LAST_LOG_SIZE);
		ftruncate(fd, 0);  // truncate to zero bytes.
		if (bytes_read > 0)
		{
			write(fd, buf.data(), KEEP_LAST_LOG_SIZE);
		}
	}
	return fd;
}

bool Log::_close(int fd)
{
	// Release the file lock we obtained in open():
	m_flock.l_type = F_UNLCK;
	m_flock.l_pid = getpid();
	if (fcntl(fd, F_SETLK, &m_flock) == -1)
	{
		int myErr = errno;
		stringstream s;
		s << "ERROR releasing file lock: ";
		s << strerror(myErr);
		s << endl;
		_RED(s.str());
	}
	close(fd);
	return true;
}

// Adapted from stacktrace.h
// NB: MUST include '-rdynamic' in CFLAGS in Makefile and then this
// will showfunc name, else just shows 'lanshark_2..._bin() [0x(ofst)]

// Returns a demangled stack backtrace of the caller function.
const string Log::GetStackTrace(const char *who)
{
	int i;

	string s("Stack Trace from ");
	s += who;
	s += ":\r\n";
	// storage array for stack trace address data
	void* addrlist[64 /*max_frames(==63)+1 */ ];
	// retrieve current stack addresses
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
	if (addrlen == 0)
	{
		s += "  <empty, possibly corrupt>\r\n";
		return s;
	}
	// Resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbollist = backtrace_symbols(addrlist, addrlen);
	
	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);
	char* tempname = funcname;
	
	// Iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	// For some reason the ARM returns 64 in addrlen and [7]->[63] are
	// exactly the same! So stop when [6] == [7]
	for (i = 1; i < addrlen - 1; i++)
	{
		if (strcmp(symbollist[i], symbollist[i+1]) == 0)
		{
			addrlen = i + 2;  // Actually shows two repeats. OK.
			break;
		}
	}
	int first = 1;
	// int last = addrlen;
	// The last entry [i == (addrlen - 1)] is always "__libc_start_main..." and I don't care
	// about that entry, do one less...
	int last = addrlen - 1;
	for (i = first; i < last; i++)
	{
		s += to_string(i);
		s += ":  ";
		// symbollist[i] raw looks like:
		// './lanshark_2.1.8.bin(_ZN15CC_Thread_Class9RunThreadEv+0x4cc) [0x6130c]'
		// This pretty-fies to look like this:
		// '  ./lanshark_2.1.8.bin : CC_Thread_Class::RunThread()+0x4cc'
		//
		// Can use arm-poky-linux-gnueabi-addr2line 0x(ofst) -e lanshark_2...bin
		// Example:
		//    "arm-poky-linux-gnueabi-addr2line 0x6130c -e lanshark_2.1.8.bin" returns:
		//    /home/user/lanshark/src/central_control.cpp:996
		// Save the absolute offset "[0x6130c]" part as (string) "0x6130c":
		char abs_ofst[32];
		char *p1 = strrchr(symbollist[i], '[');
		if (p1 != NULL)
		{
			strcpy(abs_ofst, p1 +1);  // "0x6130c]"
			abs_ofst[strlen(abs_ofst) - 1] = 0;  // "0x6130c"
		}
		else
		{
			abs_ofst[0] = 0;
		}
		
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;
		
		// Find parentheses and +address offset surrounding the mangled name:
		// "./module(function+0x15c) [0x8048a6d]"
		for (char *p = symbollist[i]; *p; ++p)
		{

			if (*p == '(')
				begin_name = p;  // ..."(function..."
			else if (*p == '+')
				begin_offset = p;  // ..."nction+0x..."
			else if (*p == ')' && begin_offset)
			{
				end_offset = p;  // ending paren
				break;
			}
		}
		
		if (begin_name && begin_offset && end_offset
			&& begin_name < begin_offset)
		{
			// "./module(function+0x15c) [0x8048a6d]"
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';
			// ==> "./module[\0]function[\0]0x15c[\0] [0x8048a6d]"
			//     begin_name --+           +-- begin_offset
			// mangled name ("function") is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset) ("0x15c").
			// The "mangled name" has extra chars at the end that tell the linker
			// what the arg types are (that is how we can overload function names).
			// Now apply __cxa_demangle():
			int status;
			char* ret = abi::__cxa_demangle(begin_name,
											tempname,
											&funcnamesize,
											&status);
			
			if (status == 0)
			{
				tempname = ret; // use possibly realloc()-ed string
				// fprintf(out, "%d:  %s+%s\n", i, tempname, begin_offset);
				s += tempname;
				s += "+";
				s += begin_offset;  // (a char *)
			}
			else
			{
				// Demangling failed, show function name as a C function with
				// no arguments.
				// fprintf(out, "%d:  %s()+%s\n", i, begin_name, begin_offset);
				s += begin_name;
				s += "()+";
				s += begin_offset;
			}
		}
		else
		{
			// Couldn't find function name start / end or offset start.
			// List the whole line.
			s += symbollist[i];
		}
		s += "\r\n";
		
		if (abs_ofst[0] != 0)
		{
			// Give a line that I can copy and execute to get a line number:
			s += "    ~/cross/addr2line ";
			s += abs_ofst;
			s += " -e shadowx\r\n";
		}
	}
	
	free(funcname);
	free(symbollist);
	return s;
}
