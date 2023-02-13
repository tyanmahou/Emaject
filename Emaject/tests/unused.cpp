#include <Emaject.hpp>

#include "catch.hpp"

namespace
{
    using emaject::Container;
    using emaject::IInstaller;
    using emaject::Injector;

    class ICounter
    {
    public:
        virtual int countUp() = 0;
    };

    struct CounterInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            // Unused
            c->bind<ICounter, 0>()
                .unused()
                .asTransient();
        }
    };

    TEST_CASE("unuused")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<ICounter>();
            REQUIRE(counter == nullptr);
        }
        {
            auto counter = injector.resolve<ICounter, 1>();
            REQUIRE(counter == nullptr);
        }
    }
}
