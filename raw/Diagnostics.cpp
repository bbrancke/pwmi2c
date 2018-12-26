// Diagnostics.cpp

#include "Diagnostics.h"

Diagnostics::Diagnostics(): ThreadRunnerBase("DiagnosticsThreadProc") { }

bool Diagnostics::Init(int outbound_fd)
{
	return m_protobufSender.Init(outbound_fd);
}

void Diagnostics::_thread_proc(void)
{
	struct statvfs        stats;
	ShadowXMessage        container_message;
	DiagnosticData      * msg;
	int32_t msg_id = 1;
	SharedMemory sharedMemory;
	m_thread_is_running = true;

	// I *read* (only) VoltageStatus from SharedMemory;
	// the value is written by BatteryChecker.
	if (!sharedMemory.Open())
	{
		LogErr(AT, "Cant Open() SharedMemory, aborting.");
		m_thread_is_running = false;
		return;
	}
	SharedData *pSharedData = sharedMemory.m_pSharedData;

	container_message.set_msg_type(MessageType::DIAGNOSTIC_DATA);
	
	msg = container_message.mutable_diagnostic_data();
	
	LogInfo("Starting DiagnosticsThreadProc...");
	int sendFail = 0;
	while (m_thread_keep_running)
	{
		statvfs(FilesysPath, &stats);
		
		msg->set_availablespace(stats.f_bavail);
		msg->set_totalspace(stats.f_blocks);
		// Note that I start long after the BatteryChecker thread has
		// started; these should always have a good current value, but user
		// display may be delayed by up to 30 seconds, 15 for me to loop and
		// 15 for the battery checker to re-check...
		msg->set_chargestatus(pSharedData->batteryChargingStatus);
		msg->set_batterystatus(pSharedData->batteryVoltageStatus);

		msg_id++;
		if (msg_id > 999999)
		{
			msg_id = 1;
		}
		container_message.set_msg_id(msg_id);
		int sleepInterval = DiagnosticsInterval;  // 15,000 ms (15 secs)
		if (!m_protobufSender.SendShadowXMessage(&container_message))
		{
			// Every once in a while send() fails with "Network Unreachable."
			// ENETUNREACH:
			//    "A write was attempted on a socket
			//     and no route to the network is present."
			// But we know the connection is OK (I think).
			// FORMER BEHAVIOR was to terminate this thread immediately here.
			// Let us try retry for awhile.
			// When C & C channel disconnects, OGDH gets a message to shut
			// me down. I think that when Android senses that diag data is
			// not coming in that it shuts down the C & C channel and we lose
			// all connections.
			sendFail++;
			if (sendFail > 20)  // Five minutes of fails...
			{
				string s("Diagnostics Thread: Can't send diag data to client: ");
				s += strerror(m_protobufSender.GetLastError());
				s += " after multiple retries. Thread proc will exit.";
				LogErr(AT, s);
				m_thread_keep_running = false;
			}
			else
			{
				stringstream ss;
				ss << "Diagnostics Thread: Could not send data to client: " <<
					strerror(m_protobufSender.GetLastError()) <<
					", fail # " << sendFail << ", will retry in five secs...";
				LogInfo(ss);
				sleepInterval = DiagnosticsIntervalShort;  // 5,000 (5 secs)
			}
		}
		else
		{
			// SendShadowXMessage() success.
			if (sendFail > 0)
			{
				LogInfo("Diagnostics Thread: Send data to client SUCCESS, resetting fail flag..");
				sendFail = 0;	
			}	
			
		}
		if (m_thread_keep_running)
		{
			Sleep_ms(sleepInterval /* 15,000 milliseconds or 5,000 if Short */);
		}
	}
	
	LogInfo("Exiting DiagnosticsThreadProc...");

	m_thread_is_running = false;

}
