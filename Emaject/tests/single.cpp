#include <Emaject.hpp>
#include <iostream>
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
    class IPrinter
    {
    public:
        virtual void println(std::string_view str) const = 0;
    };
    class PrintCounter : public ICounter, public IPrinter
    {
        int m_count;
    public:
        int countUp() override
        {
            return ++m_count;
        }
        void println(std::string_view str) const override
        {
            std::cout << "[single]" << str << " " << m_count << std::endl;
        }
    };

    struct CounterInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<ICounter>()
                .to<PrintCounter>()
                .asSingle();

            // failed
            c->bind<ICounter, 1>()
                .to<PrintCounter>()
                .asCached();

            // failed
            c->bind<IPrinter>()
                .to<PrintCounter>()
                .asTransient();
        }
    };

    TEST_CASE("single")
    {
        Injector injector;
        injector.install<CounterInstaller>();

        {
            auto counter = injector.resolve<ICounter, 0>(); // new instance(single)
            REQUIRE(counter != nullptr);
            REQUIRE(counter->countUp() == 1);
        }
        {
            auto counter1 = injector.resolve<ICounter, 0>(); // cached
            auto counter2 = injector.resolve<ICounter, 0>(); // cached
            REQUIRE(counter1 == counter2);
        }
        {
            auto counter = injector.resolve<ICounter, 1>(); // failed
            REQUIRE(counter == nullptr);
        }
        {
            auto printer = injector.resolve<IPrinter>(); // failed
            REQUIRE(printer == nullptr);
        }
    }
}
