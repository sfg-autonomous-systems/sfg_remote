#pragma

#include <chrono>

namespace sfg_teleop_interface::app
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    static std::int64_t now_ms()
    {
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
}