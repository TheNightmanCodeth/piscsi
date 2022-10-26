//---------------------------------------------------------------------------
//
// SCSI Target Emulator RaSCSI Reloaded
// for Raspberry Pi
//
// Copyright (C) 2021-22 Uwe Seimet
//
//---------------------------------------------------------------------------

#include "rascsi_version.h"
#include "rasutil.h"
#include <sstream>

using namespace std;
using namespace rascsi_interface;

bool ras_util::GetAsInt(const string& value, int& result)
{
	if (value.find_first_not_of("0123456789") != string::npos) {
		return false;
	}

	try {
		auto v = stoul(value);
		result = (int)v;
	}
	catch(const invalid_argument&) {
		return false;
	}
	catch(const out_of_range&) {
		return false;
	}

	return true;
}


string ras_util::Banner(const string& app)
{
	ostringstream s;

	s << "SCSI Target Emulator RaSCSI " << app << " version " << rascsi_get_version_string()
			<< "  (" << __DATE__ << ' ' << __TIME__ << ")\n";
	s << "Powered by XM6 TypeG Technology / ";
	s << "Copyright (C) 2016-2020 GIMONS\n";
	s << "Copyright (C) 2020-2022 Contributors to the RaSCSI Reloaded project\n";

	return s.str();
}

string ras_util::ListDevices(const list<PbDevice>& pb_devices)
{
	if (pb_devices.empty()) {
		return "No devices currently attached.\n";
	}

	ostringstream s;
	s << "+----+-----+------+-------------------------------------\n"
			<< "| ID | LUN | TYPE | IMAGE FILE\n"
			<< "+----+-----+------+-------------------------------------\n";

	list<PbDevice> devices = pb_devices;
	devices.sort([](const auto& a, const auto& b) { return a.id() < b.id() || a.unit() < b.unit(); });

	for (const auto& device : devices) {
		string filename;
		switch (device.type()) {
			case SCBR:
				filename = "X68000 HOST BRIDGE";
				break;

			case SCDP:
				filename = "DaynaPort SCSI/Link";
				break;

			case SCHS:
				filename = "Host Services";
				break;

			case SCLP:
				filename = "SCSI Printer";
				break;

			default:
				filename = device.file().name();
				break;
		}

		s << "|  " << device.id() << " |   " << device.unit() << " | " << PbDeviceType_Name(device.type()) << " | "
				<< (filename.empty() ? "NO MEDIUM" : filename)
				<< (!device.status().removed() && (device.properties().read_only() || device.status().protected_()) ? " (READ-ONLY)" : "")
				<< '\n';
	}

	s << "+----+-----+------+-------------------------------------\n";

	return s.str();
}

string ras_util::GetExtensionLowerCase(const string& filename)
{
	string ext;
	if (const size_t separator = filename.rfind('.'); separator != string::npos) {
		ext = filename.substr(separator + 1);
	}
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

	return ext;
}

// Pin the thread to a specific CPU
// TODO Check whether just using a single CPU really makes sense
void ras_util::FixCpu(int cpu)
{
#ifdef __linux__
	// Get the number of CPUs
	cpu_set_t mask;
	CPU_ZERO(&mask);
	sched_getaffinity(0, sizeof(cpu_set_t), &mask);
	const int cpus = CPU_COUNT(&mask);

	// Set the thread affinity
	if (cpu < cpus) {
		CPU_ZERO(&mask);
		CPU_SET(cpu, &mask);
		sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	}
#endif
}