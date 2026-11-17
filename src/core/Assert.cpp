#include "core/Assert.h"

#include "core/Log.h"

namespace dl::assertion
{
namespace
{
Handler& H()
{
    static Handler handler = nullptr;
    return handler;
}
} // namespace

void SetHandler(Handler handler)
{
    H() = handler;
}

bool Fail(const char* file, int line, const char* expr, const char* message)
{
    if (H() != nullptr)
    {
        H()(file, line, expr, message);
        return false;
    }
    DL_LOG_ERROR("Assert", "%s:%d  (%s)  %s", file, line, expr, message);
#if defined(_DEBUG)
    return true;
#else
    return false;
#endif
}
} // namespace dl::assertion
