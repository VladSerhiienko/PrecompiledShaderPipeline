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
    cxxopts::Options Options;               /* User parameters */
    tf::Taskflow Taskflow;                  /* Task flow */

    ImplementedAppState(int args, const char** argv);
    virtual ~ImplementedAppState();
};
} // namespace
} // namespace apemode

static apemode::ImplementedAppState* gState = nullptr;

apemode::AppState* apemode::AppState::Get() { return gState; }
spdlog::logger* apemode::AppState::GetLogger() { return gState->Logger.get(); }
cxxopts::Options* apemode::AppState::GetArgs() { return &gState->Options; }
tf::Taskflow* apemode::AppState::GetDefaultTaskflow() { return &gState->Taskflow; }

void apemode::AppState::OnMain(int args, const char** ppArgs) {
    if (nullptr == gState) gState = new ImplementedAppState(args, ppArgs);
}

void apemode::AppState::OnExit() {
    if (nullptr != gState) {
        delete gState;
        gState = nullptr;
    }
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

    constexpr std::string_view loggerName = "appstate";

    auto logger = spdlog::get(loggerName.data());
    if (!logger) { logger = spdlog::create<>(loggerName.data(), sinks.begin(), sinks.end()); }

    logger->set_level(lvl);
    spdlog::set_pattern("[%T.%f:%t:%L] %v");
    return logger;
}

apemode::ImplementedAppState::ImplementedAppState(int argc, const char** argv)
    : Logger(nullptr), Options(argv[0]), Taskflow("Default") {
    Logger = CreateLogger(spdlog::level::trace, ComposeLogFile());

    Options.add_options("main")("i,input-file", "Input", cxxopts::value<std::string>());
    Options.add_options("main")("o,output-file", "Output", cxxopts::value<std::string>());
    Options.add_options("main")("m,mode", "Mode", cxxopts::value<std::string>()->default_value("build-collection"));
    Options.add_options("main")("a,add-path", "Add path", cxxopts::value<std::vector<std::string>>());
    Options.parse(argc, argv);
}

apemode::ImplementedAppState::~ImplementedAppState() = default;
