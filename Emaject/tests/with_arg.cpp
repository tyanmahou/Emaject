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
        Counter(int count = 0):
            m_count(count)
        {}

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
                .fromArgs(100)
                .asTransient();
        }
    };

    TEST_CASE("with_arg")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<ICounter>();
            REQUIRE(counter->countUp() == 101);
        }
    }
}
