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

    struct Test
    {
        [[INJECT(c1)]]
        std::shared_ptr<ICounter> c1;
        [[INJECT(c1_2)]]
        std::shared_ptr<ICounter> c1_2;
        [[INJECT(c2, 2)]]
        std::shared_ptr<ICounter> c2;
        [[INJECT(c2_2, 2)]]
        std::shared_ptr<ICounter> c2_2;

        [[INJECT(c3)]]
        std::shared_ptr<Counter> c3;
    };
    struct CounterInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<ICounter>()
                .to<Counter>()
                .asCached();

            c->bind<ICounter, 2>()
                .to<Counter>()
                .asCached();

            c->bind<Counter>()
                .asCached();
        }
    };
    TEST_CASE("cached")
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
        {
            auto test = injector.resolve<Test>();
            REQUIRE(test->c1 != nullptr);
            REQUIRE(test->c2 != nullptr);
            REQUIRE(test->c3 != nullptr);

            REQUIRE(test->c1 == test->c1_2);
            REQUIRE(test->c1 != test->c2);
            REQUIRE(test->c1 != test->c2_2);
            REQUIRE(test->c1 != test->c3);

            REQUIRE(test->c2 == test->c2_2);
            REQUIRE(test->c3 == test->c3);
        }
    }
}