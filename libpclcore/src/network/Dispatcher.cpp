#include "pclcore/network/Dispatcher.hpp"

namespace pclcore::network {

namespace {

SynchronousDispatcher s_default_dispatcher;
Dispatcher*           s_dispatcher = &s_default_dispatcher;

}  // anonymous namespace

Dispatcher& get_dispatcher()
{
    return *s_dispatcher;
}

void set_dispatcher(Dispatcher& d)
{
    s_dispatcher = &d;
}

}  // namespace pclcore::network
