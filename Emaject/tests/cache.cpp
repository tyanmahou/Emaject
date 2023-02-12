#include <Emaject.hpp>

#include <string_view>
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

    class Counter : public ICounter
    {
        int m_count;
    public:
        int countUp() override
        {
            return ++m_count;
        }
    };

    struct CounterInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<ICounter>()
                .to<Counter>()
                .asCache();
        }
    };
    TEST_CASE("catch")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<ICounter>();  // new instance
            REQUIRE(counter->countUp() == 1);
        }
        {
            auto counter = injector.resolve<ICounter>();  // used cache
            REQUIRE(counter->countUp() == 2);
        }
    }
}