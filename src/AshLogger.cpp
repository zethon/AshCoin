#include <boost/process/environment.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/cfg/env.h>
#include "AshLogger.h"

namespace ash
{

constexpr const char* GLOBAL_LOGGER = "Ash";

[[maybe_unused]] SpdLogPtr rootLogger()
{
    SpdLogPtr root = spdlog::get(GLOBAL_LOGGER);

    if (!root)
    {
        root = spdlog::stdout_color_mt(ash::GLOBAL_LOGGER);

#ifdef RELEASE
        spdlog::set_level(spdlog::level::off);
#else
        spdlog::set_level(spdlog::level::trace);
#endif
    }

    return root;
}

SpdLogPtr initializeLogger(const std::string& name)
{
    ash::rootLogger();

    SpdLogPtr logger = spdlog::get(name);
    if (!logger)
    {
        logger = spdlog::get(GLOBAL_LOGGER)->clone(name);
        spdlog::register_logger(logger);
    }

    // reload the logger levels
    spdlog::cfg::load_env_levels();
    return logger;
}

} // namespace
