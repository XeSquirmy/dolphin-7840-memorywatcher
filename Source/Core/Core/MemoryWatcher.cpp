// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "Common/FileUtil.h"
#include "Common/Thread.h"
#include "Core/MemoryWatcher.h"
#include "Core/HW/Memmap.h"

// We don't want to kill the cpu, so sleep for this long after polling.
static const int SLEEP_DURATION = 2; // ms

MemoryWatcher::MemoryWatcher()
{
	if (!LoadAddresses(File::GetUserPath(F_MEMORYWATCHERLOCATIONS_IDX)))
		return;
	if (!OpenSocket(File::GetUserPath(F_MEMORYWATCHERSOCKET_IDX)))
		return;
	m_running = true;
	m_watcher_thread = std::thread(&MemoryWatcher::WatcherThread, this);
}

MemoryWatcher::~MemoryWatcher()
{
	if (!m_running)
		return;

	m_running = false;
	m_watcher_thread.join();
	close(m_fd);
}

bool MemoryWatcher::LoadAddresses(const std::string& path)
{
	std::ifstream locations(path);
	if (!locations)
		return false;

	std::string line;
	while (std::getline(locations, line))
		ParseLine(line);

	return m_values.size() > 0;
}

void MemoryWatcher::ParseLine(const std::string& line)
{
	m_values[line] = 0;
	m_addresses[line] = std::vector<u32>();

	std::stringstream offsets(line);
	offsets >> std::hex;
	u32 offset;
	while (offsets >> offset)
		m_addresses[line].push_back(offset);
}

bool MemoryWatcher::OpenSocket(const std::string& path)
{
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sun_family = AF_UNIX;
	strncpy(m_addr.sun_path, path.c_str(), sizeof(m_addr.sun_path) - 1);

	m_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	return m_fd >= 0;
}

u32 MemoryWatcher::ChasePointer(const std::string& line)
{
	u32 value = 0;
	for (u32 offset : m_addresses[line])
		value = Memory::Read_U32(value + offset);
	return value;
}

std::string MemoryWatcher::ComposeMessage(const std::string& line, u32 value)
{
	std::stringstream message_stream;
	message_stream << line << '\n' << std::hex << value;
	return message_stream.str();
}

void MemoryWatcher::WatcherThread()
{
	while (m_running)
	{
		for (auto& entry : m_values)
		{
			std::string address = entry.first;
			u32& current_value = entry.second;

			u32 new_value = ChasePointer(address);
			if (new_value != current_value)
			{
				// Update the value
				current_value = new_value;
				std::string message = ComposeMessage(address, new_value);
				sendto(
					m_fd,
					message.c_str(),
					message.size() + 1,
					0,
					reinterpret_cast<sockaddr*>(&m_addr),
					sizeof(m_addr));
			}
		}
		Common::SleepCurrentThread(SLEEP_DURATION);
	}
}
