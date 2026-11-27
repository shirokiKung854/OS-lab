#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <sys/statvfs.h>

using namespace std;
namespace fs = std::filesystem;

static string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static string readFirstMatch(const string &path, const string &key) {
    ifstream file(path);
    string line;
    while (getline(file, line)) {
        size_t pos = line.find(key);
        if (pos != string::npos) {
            size_t colon = line.find(':', pos);
            if (colon != string::npos)
                return trim(line.substr(colon + 1));
        }
    }
    return "Unknown";
}

static string getCpuName() {
    return readFirstMatch("/proc/cpuinfo", "model name");
}

static int getCpuCores() {
    ifstream file("/proc/cpuinfo");
    string line;
    int cores = 0;
    while (getline(file, line))
        if (line.rfind("processor", 0) == 0) cores++;
    return cores;
}

static void showMemory() {
    ifstream file("/proc/meminfo");
    string line;
    long long total = 0, freeMem = 0, available = 0;

    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        long long value;
        string unit;
        iss >> key >> value >> unit;

        if (key == "MemTotal:") total = value;
        else if (key == "MemFree:") freeMem = value;
        else if (key == "MemAvailable:") available = value;
    }

    auto toGB = [](long long kb) { return kb / 1024.0 / 1024.0; };

    cout << fixed << setprecision(2);
    cout << "  Total Memory    : " << toGB(total) << " GB\n";
    cout << "  Available Memory: " << toGB(available) << " GB\n";
    cout << "  Free Memory     : " << toGB(freeMem) << " GB\n";
    cout << "  Used Memory     : " << toGB(total - available) << " GB\n";
}

static void showOsInfo() {
    ifstream file("/etc/os-release");
    if (!file.is_open()) {
        cout << "  OS: Unknown\n";
        return;
    }

    string line, name, version, pretty;
    while (getline(file, line)) {
        auto strip = [](string s) {
            if (!s.empty() && s.front() == '"') s.erase(0, 1);
            if (!s.empty() && s.back() == '"') s.pop_back();
            return s;
        };
        if (line.rfind("NAME=", 0) == 0) name = strip(line.substr(5));
        else if (line.rfind("VERSION=", 0) == 0) version = strip(line.substr(8));
        else if (line.rfind("PRETTY_NAME=", 0) == 0) pretty = strip(line.substr(12));
    }

    cout << "  OS Name    : " << (name.empty() ? "Unknown" : name) << "\n";
    cout << "  OS Version : " << (version.empty() ? "Unknown" : version) << "\n";
    if (!pretty.empty())
        cout << "  Full Name  : " << pretty << "\n";
}

static void showDisk() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        cout << "  Cannot read disk information\n";
        return;
    }

    auto toGB = [](unsigned long long bytes) { return bytes / 1024.0 / 1024.0 / 1024.0; };

    unsigned long long total = stat.f_blocks * stat.f_frsize;
    unsigned long long freeSpace = stat.f_bfree * stat.f_frsize;
    unsigned long long avail = stat.f_bavail * stat.f_frsize;
    unsigned long long used = total - freeSpace;

    cout << fixed << setprecision(2);
    cout << "  Filesystem : /\n";
    cout << "  Total      : " << toGB(total) << " GB\n";
    cout << "  Used       : " << toGB(used) << " GB\n";
    cout << "  Available  : " << toGB(avail) << " GB\n";
    cout << "  Usage      : " << (total ? (used * 100.0 / total) : 0.0) << " %\n";
}

static void showProcesses() {
    struct ProcInfo {
        int pid;
        string name;
    };
    vector<ProcInfo> processes;

    for (const auto &entry : fs::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;

        string filename = entry.path().filename().string();
        if (filename.find_first_not_of("0123456789") != string::npos) continue;

        ifstream comm(entry.path() / "comm");
        string name;
        if (getline(comm, name))
            processes.push_back({stoi(filename), trim(name)});
    }

    sort(processes.begin(), processes.end(),
         [](const ProcInfo &a, const ProcInfo &b) { return a.pid < b.pid; });

    cout << "  Total processes: " << processes.size() << "\n\n";
    cout << "  " << left << setw(8) << "PID" << "NAME\n";
    cout << "  " << string(40, '-') << "\n";
    for (const auto &p : processes)
        cout << "  " << left << setw(8) << p.pid << p.name << "\n";
}

int main() {
    cout << "========================================\n";
    cout << "         SYSTEM INFORMATION\n";
    cout << "========================================\n\n";

    cout << "[1] CPU\n";
    cout << "  CPU Name  : " << getCpuName() << "\n";
    cout << "  CPU Cores : " << getCpuCores() << "\n\n";

    cout << "[2] MEMORY\n";
    showMemory();
    cout << "\n";

    cout << "[3] OPERATING SYSTEM\n";
    showOsInfo();
    cout << "\n";

    cout << "[4] DISK STORAGE\n";
    showDisk();
    cout << "\n";

    cout << "[5] RUNNING PROCESSES\n";
    showProcesses();

    return 0;
}
