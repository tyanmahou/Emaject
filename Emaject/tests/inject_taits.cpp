#include <Emaject.hpp>

#include <iostream>
#include <string_view>
#include "catch.hpp"

namespace
{
    using emaject::Container;
    using emaject::IInstaller;
    using emaject::Injector;

    class IPrinter
    {
    public:
        virtual void println(std::string_view str) const = 0;
    };

    class CoutPrinter : public IPrinter
    {
    public:
        void println(std::string_view str) const override
        {
            std::cout << str << std::endl;
        }
    };

    struct CoutInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<IPrinter>()
                .to<CoutPrinter>()
                .asCache();
        }
    };

    class HelloWorld
    {
    public:
        HelloWorld()
        {}

        void greet() const
        {
            m_printer->println("[inject trairs] Hello World");
        }

        void setPrinter(std::shared_ptr<IPrinter> printer)
        {
            m_printer = printer;
        }
    private:
        std::shared_ptr<IPrinter> m_printer;
    };
}

namespace emaject
{
    template<>
    struct InjectTraits<HelloWorld>
    {
        void onInject(HelloWorld* value, Container* c)
        {
            value->setPrinter(c->resolve<IPrinter>());
        }
    };
}

namespace
{
    TEST_CASE("inject trairs")
    {
        Injector injector;
        injector.install<CoutInstaller>();

        auto helloWorld = injector.resolve<HelloWorld>();
        helloWorld->greet();
    }
}