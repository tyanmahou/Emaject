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
            c->bind<ICounter>()
                .to<Counter>()
                .asSingle();

            // compile error "don't use ID"
            //c->bind<ICounter, 1>()
            //    .to<Counter>()
            //    .asSingle();
        }
    };

    TEST_CASE("single")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<ICounter, 0>(); // new instance
            REQUIRE(counter->countUp() == 1);
        }
        {
            auto counter = injector.resolve<ICounter, 1>(); // used cache
            REQUIRE(counter->countUp() == 2);
        }
    }
}
