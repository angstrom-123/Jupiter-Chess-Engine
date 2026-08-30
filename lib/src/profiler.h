#pragma once

#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

namespace chrono = std::chrono;
namespace fs = std::filesystem;

static const fs::path PROFILE_FILE_PATH = fs::path(__FILE__).parent_path().parent_path() / "profile.json";
static const auto TIMESTAMP_OFFSET = chrono::time_point_cast<chrono::microseconds>(chrono::high_resolution_clock::now()).time_since_epoch().count();

class Profiler {
public:
    static std::ofstream& GetFile() 
    {
        thread_local std::ofstream file;

        if (!file.is_open())
            file.open(PROFILE_FILE_PATH, std::ios::app);

        if (file.tellp() == 0)
            file << "[";

        return file;
    }

    static void EndSession()
    {
        Profiler::GetFile() 
            << "{"
            << "\"name\": \"end\","
            << "\"cat\": \"special\","
            << "\"ph\": \"X\","
            << "\"ts\": \"0\","
            << "\"dur\": \"0\","
            << "\"pid\": \"0000\","
            << "\"tid\": \"0\""
            << "}]";
        Profiler::GetFile().close();
    }
};

class ProfileInstrumenter {
public:
    ProfileInstrumenter(const char *functionName, const char *fileName, int64_t lineNumber)
    {
        m_FunctionName = functionName;
        m_FileName = fileName;
        m_LineNumber = lineNumber;
        m_StartTimepoint = chrono::high_resolution_clock::now();
    }

    ~ProfileInstrumenter()
    {
        uint64_t start = chrono::time_point_cast<chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();

        auto endTimepoint = chrono::high_resolution_clock::now();
        uint64_t end = chrono::time_point_cast<chrono::microseconds>(endTimepoint).time_since_epoch().count();

        uint32_t thread = std::hash<std::thread::id>{}(std::this_thread::get_id());

        Profiler::GetFile() 
            << "{"
            << "\"name\": \"" << fs::path(m_FileName).filename().c_str() << ":" << m_FunctionName << "\","
            << "\"cat\": \"default\","
            << "\"ph\": \"X\","
            << "\"ts\": \"" << start - TIMESTAMP_OFFSET << "\","
            << "\"dur\": \"" << std::max(end - start, static_cast<uint64_t>(0)) << "\","
            << "\"pid\": \"0000\","
            << "\"tid\": \"" << thread << "\""
            << "},"
            << std::endl;
    }

private:
    const char *m_FunctionName{nullptr};
    const char *m_FileName{nullptr};
    int64_t m_LineNumber{0};
    chrono::time_point<chrono::high_resolution_clock> m_StartTimepoint;
};

