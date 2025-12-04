#include <iuring/ShellUtils.hpp>

namespace iuring::shell
{

void run_cmd(const std::string& cmd, logging::ILogger& logger, RunOpt opt)
{
    LOG_INFO(logger, "run_cmd: {}", cmd);
    int ret = system(cmd.c_str());
    if (ret != 0)
    {
        switch (opt)
        {
        case RunOpt::ABORT_ON_ERROR:
            LOG_ERROR(logger, "failed to run cmd: {}", cmd.c_str());
            abort();

        case RunOpt::LOG_ERROR_AS_WARNING:
            LOG_INFO(logger, "failed to run cmd: {}", cmd.c_str());
            break;
        }
    }
}

} // namespace iuring::shell