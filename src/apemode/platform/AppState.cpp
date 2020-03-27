#include "AppState.h"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iomanip>
#include <iostream>

#pragma warning(push)
#pragma warning(disable : 4244)
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#pragma warning(pop)

namespace apemode {
namespace {
class ImplementedAppState : public AppState {
public:
    std::shared_ptr<spdlog::logger> Logger; /* Prints to console and file */
    argh::parser Cmdl;                      /* User parameters */
    tf::Taskflow Taskflow;                  /* Task flow */

    ImplementedAppState(int args, const char** argv);
    virtual ~ImplementedAppState();
};
} // namespace
} // namespace apemode

static apemode::ImplementedAppState* gState = nullptr;

apemode::AppState* apemode::AppState::Get() { return gState; }
spdlog::logger* apemode::AppState::GetLogger() { return gState->Logger.get(); }
argh::parser* apemode::AppState::GetArgs() { return &gState->Cmdl; }
tf::Taskflow* apemode::AppState::GetDefaultTaskflow() { return &gState->Taskflow; }

void apemode::AppState::OnMain(int args, const char** ppArgs) {
    if (nullptr == gState) gState = new ImplementedAppState(args, ppArgs);
}

void apemode::AppState::OnExit() {
    if (nullptr != gState) { delete gState; }
}

std::string ComposeLogFile() {
    tm* pCurrentTime = nullptr;
    time_t currentSystemTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

#if _WIN32
    tm currentTime;
    localtime_s(&currentTime, &currentSystemTime);
    pCurrentTime = &currentTime;
#else
    pCurrentTime = localtime(&currentSystemTime);
#endif

    std::stringstream currentTimeStrStream;
    currentTimeStrStream << std::put_time(pCurrentTime, "%F-%T-");
    currentTimeStrStream << currentSystemTime;

    std::string curentTimeStr = currentTimeStrStream.str();
    std::replace(curentTimeStr.begin(), curentTimeStr.end(), ' ', '-');
    std::replace(curentTimeStr.begin(), curentTimeStr.end(), ':', '-');

    std::string logFile = "log-";
    logFile += curentTimeStr;
    logFile += ".txt";

    return logFile;
}

std::shared_ptr<spdlog::logger> CreateLogger(spdlog::level::level_enum lvl, std::string logFile) {
    std::vector<spdlog::sink_ptr> sinks {
#if _WIN32
        std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::msvc_sink_mt>(),
            std::make_shared<spdlog::sinks::simple_file_sink_mt>(logFile.c_str())
#else
        std::make_shared<spdlog::sinks::stdout_sink_mt>(),
            std::make_shared<spdlog::sinks::simple_file_sink_mt>(logFile.c_str())
#endif
    };

    auto logger = spdlog::create<>("viewer", sinks.begin(), sinks.end());
    logger->set_level(lvl);
    spdlog::set_pattern("[%T.%f] [%t] [%L] %v");
    return logger;
}

apemode::ImplementedAppState::ImplementedAppState(int argc, const char** argv)
    : Logger(nullptr), Cmdl(), Taskflow("Default") {
    Logger = CreateLogger(spdlog::level::trace, ComposeLogFile());
    Cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

    // Logger->info("Input argumets:");
    // for (int i = 0; i < argc; ++i) { Logger->info("#{} = {}", i, argv[i]); }
}

apemode::ImplementedAppState::~ImplementedAppState() = default;
