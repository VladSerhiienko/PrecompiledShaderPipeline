#pragma once

#include <taskflow/taskflow.hpp>

#pragma warning(push)
#pragma warning(disable : 4244)

#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>

#include <cxxopts.hpp>

#pragma warning(pop)

#include <utility>

namespace apemode {

/**
 * @class AppStoredValue
 * @brief Contains a value created and stored by the user
 */
class AppStoredValue {
public:
    virtual ~AppStoredValue() = default;
};

/**
 * @class AppState
 * @brief Contains members allowed for global access across the App classes
 */
class AppState {
public:
    virtual ~AppState() = default;

    /**
     * @return Application state instance.
     * @note Returns null before the application creation, or after its destruction.
     */
    static AppState* Get();
    static void OnMain(int argc, const char** argv);
    static void OnExit();

    spdlog::logger* GetLogger();
    cxxopts::Options* GetArgs();
    tf::Taskflow* GetDefaultTaskflow();
};

struct AppStateExitGuard {
    ~AppStateExitGuard() { AppState::OnExit(); }
};

enum LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Err = 4,
    Critical = 5,
};

template <typename... Args>
inline void Log(LogLevel eLevel, const char* szFmt, Args&&... args) {
    if (auto pAppState = AppState::Get())
        if (auto pLogger = pAppState->GetLogger()) {
            pLogger->log(static_cast<spdlog::level::level_enum>(eLevel), szFmt, std::forward<Args>(args)...);
        }
}

template <typename... Args>
inline void LogInfo(const char* szFmt, Args&&... args) {
    if (auto pAppState = AppState::Get())
        if (auto pLogger = pAppState->GetLogger()) { pLogger->info(szFmt, std::forward<Args>(args)...); }
}

template <typename... Args>
inline void LogError(const char* szFmt, Args&&... args) {
    if (auto pAppState = AppState::Get())
        if (auto pLogger = pAppState->GetLogger()) { pLogger->error(szFmt, std::forward<Args>(args)...); }
}

template <typename... Args>
inline void LogWarn(const char* szFmt, Args&&... args) {
    if (auto pAppState = AppState::Get())
        if (auto pLogger = pAppState->GetLogger()) { pLogger->warn(szFmt, std::forward<Args>(args)...); }
}

} // namespace apemode
