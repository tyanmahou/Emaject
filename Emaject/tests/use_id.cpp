#include <Emaject.hpp>

#include <iostream>
#include <cstdio>
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
            std::cout << str << " for cout" << std::endl;
        }
    };

    class PrintfPrinter : public IPrinter
    {
    public:
        void println(std::string_view str) const override
        {
            std::printf("%s for printf\n", str.data());
        }
    };

    struct CoutInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            // ID = 0 -> CoutPrinter
            c->bind<IPrinter, 0>()
                .to<CoutPrinter>()
                .asCached();
        }
    };
    struct PrintfInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            // ID = 1 -> PrintfPrinter
            c->bind<IPrinter, 1>()
                .to<PrintfPrinter>()
                .asCached();
        }
    };
    class HelloWorld
    {
    public:
        HelloWorld() = default;

        void greet() const
        {
            m_printer0->println("Hello World");
            m_printer1->println("Hello World");
        }
    private:
        [[INJECT(m_printer0)]] // ID = 0
        std::shared_ptr<IPrinter> m_printer0;

        [[INJECT(m_printer1, 1)]]
        std::shared_ptr<IPrinter> m_printer1;
    };

    TEST_CASE("use_id")
    {
        Injector injector;
        injector
            .install<CoutInstaller>()
            .install<PrintfInstaller>();

        auto helloWorld = injector.instantiate<HelloWorld>();
        helloWorld->greet();
    }
}
