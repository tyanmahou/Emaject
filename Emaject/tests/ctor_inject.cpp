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
        }
    };

    struct CoutInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<IPrinter>()
                .to<CoutPrinter>()
                .asCached();
        }
    };

    class HelloWorld
    {
    public:
        INJECT_CTOR(HelloWorld(std::shared_ptr<IPrinter> printer)) :
            m_printer(printer)
        {}

        void greet() const
        {
            m_printer->println("[ctor_inject] Hello World");
        }
    private:
        std::shared_ptr<IPrinter> m_printer;
    };

    TEST_CASE("ctor_inject")
    {
        Injector injector;
        injector.install<CoutInstaller>();

        auto helloWorld = injector.instantiate<HelloWorld>();
        helloWorld->greet();
    }
}
