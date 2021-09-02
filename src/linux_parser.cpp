#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <numeric>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

static float constexpr kb_to_mb{0.001};

#include <iostream> // for debug, remove later.
// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// Done: Read and return the system memory utilization
float LinuxParser::MemoryUtilization() 
{
  //MemTotal:       13040328 kB
  //MemFree:        11108252 kB
  string line, key, value;
  float mem_total, mem_free;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) 
  {
    while (std::getline(stream, line))
    {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      
      while (linestream >> key >> value)
      {
        if (key == "MemTotal") {     
          mem_total = std::stof(value);
        }
        else if (key == "MemFree")
        {
          mem_free = std::stof(value);
        }
      }
    }
  }

  return (mem_total - mem_free)/mem_total;   
}

// Done: Read and return the system uptime
long LinuxParser::UpTime()  // need to define helper function in format.cpp
{ 
  string uptime, idle_time;
  string line;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) 
  {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> uptime >> idle_time;
  }
  return std::stol(uptime); 
}

// Done: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() 
{ 
  return sysconf(_SC_CLK_TCK) * UpTime();
}

// TODO: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::ActiveJiffies(int pid[[maybe_unused]]) { return 0; }

namespace
{
std::unordered_map<std::string, long int> readProcessorInformation()
{
  std::unordered_map<std::string, long int> info;
  std::ifstream stream(LinuxParser::kProcDirectory + LinuxParser::kStatFilename);

  string cpu;
  string line;
  if (stream.is_open()) 
  {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> cpu >> info["user"] >> info["nice"] >> info["system"] >> info["idle"] >> info["iowait"] >> info["irq"] >> info["softirq"] >> info["steal"] >> info["guest"] >> info["guest_nice"];
  }

  return info;
}
} 
// TODO: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies()
 { 
  auto info = readProcessorInformation();
  long int sum = std::accumulate(info.begin(), info.end(), 0L, [](int a,  std::pair<std::string, long int> b) {return a + b.second;});
  return sum;
 }

// TODO: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() 
{ 
  auto info = readProcessorInformation();
  return info["idle"] + info["iowait"];

}

// TODO: Read and return CPU utilization
float LinuxParser::CpuUtilization() 
{ 
  // NonIdle = user + nice + system + irq + softirq + steal
  // Total = Idle + NonIdle
  // CPU_Percentage = (total - idle)/total
  float total = static_cast<float>(LinuxParser::ActiveJiffies());
  float idle = static_cast<float>(LinuxParser::IdleJiffies());

  return (total - idle) / total;
}

float LinuxParser::CpuUtilization(int pid) 
{
  // #14 utime - CPU time spent in user code, measured in clock ticks
  // #15 stime - CPU time spent in kernel code, measured in clock ticks
  // #16 cutime - Waited-for children's CPU time spent in user code (in clock ticks)
  // #17 cstime - Waited-for children's CPU time spent in kernel code (in clock ticks)
  // #22 starttime - Time when the process started, measured in clock ticks
  float uptime = UpTime();
  float utime, stime, cutime, cstime, starttime;
  
  string value, line;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatFilename);
  
  if (stream.is_open()) 
  {
    std::getline(stream, line);
    
    std::istringstream linestream(line);
    int element_counter{0};
    while (linestream >> value)
    {
      ++element_counter;
      if (element_counter==14) {     
        utime = std::stof(value);
      }
      if (element_counter==15)
      {
        stime = std::stof(value);
      }
      else if (element_counter==16)
      {
        cutime = std::stof(value);
      }
      else if (element_counter==17)
      {
        cstime = std::stof(value);
      }
      else if (element_counter==22)
      {
        starttime = std::stof(value);
        break;
      }
    }
   
  }

  float total_time = utime + stime + cutime + cstime;
  float hertz = sysconf(_SC_CLK_TCK);
  float seconds = uptime - (starttime / hertz);

  return (total_time / hertz) / seconds;
}

namespace 
{
int getStatValueFor(std::string target_key)
{
  string line, key;
  int value;
  std::ifstream stream(LinuxParser::kProcDirectory + LinuxParser::kStatFilename);
  if (stream.is_open()) 
  {
    while (std::getline(stream, line))
    {
      std::istringstream linestream(line);
      while (linestream >> key >> value)
      {
        if (key == target_key) {
          return value;
        }
      }
    }
  }

  return 0;
}
} // anonymous namespace

// Done: Read and return the total number of processes
int LinuxParser::TotalProcesses() 
{ 
  return getStatValueFor("processes");
}

// Done: Read and return the number of running processes
int LinuxParser::RunningProcesses() 
{
  return getStatValueFor("procs_running");
}

// TODO: Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid) 
{
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kCmdlineFilename);
  string line;
   if (stream.is_open()) 
   {
     std::getline(stream, line);
   }

  return line;
}

// TODO: Read and return the memory used by a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Ram(int pid) 
{ 
  string line, key;
  float vm_size;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatusFilename);
  if (stream.is_open()) 
  {
    while (std::getline(stream, line))
    {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      
      while (linestream >> key >> vm_size)
      {
        if (key == "VmSize") {     
          return std::to_string(static_cast<long int>(vm_size * kb_to_mb));
        }
      }
    }
  }
  return string();
}

// TODO: Read and return the user ID associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Uid(int pid) 
{
  string line, key, value;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatusFilename);
  if (stream.is_open()) 
  {
    while (std::getline(stream, line))
    {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      
      while (linestream >> key >> value)
      {
        if (key == "Uid") {     
          return value;
        }
      }
    }
  }
  return string();   
}

// TODO: Read and return the user associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::User(int pid) 
{ 
  string line, username, extra_token, uid;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) 
  {
    while (std::getline(stream, line))
    {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      
      while (linestream >> username >> extra_token >> uid)
      {
        if (uid == Uid(pid)) {     
          return username;
        }
      }
    }
  }
  return string();   
}

// TODO: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid) 
{
  string value, line;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatFilename);
  if (stream.is_open()) 
  {
    std::getline(stream, line);
    
    std::istringstream linestream(line);
    int element_counter{0};
    while (linestream >> value)
    {
      ++element_counter;
      if (element_counter==22) {     
        return UpTime() - std::stof(value)/sysconf(_SC_CLK_TCK);
      }
    }
   
  }
  return 0;
}
