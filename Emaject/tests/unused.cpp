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
            // Unused
            c->bind<Counter, 0>()
                .unused()
                .asTransient();
        }
    };

    TEST_CASE("unuused")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<Counter>();
            REQUIRE(counter == nullptr);
        }
        {
            auto counter = injector.resolve<Counter, 1>();
            REQUIRE(counter != nullptr);
        }
    }
}
