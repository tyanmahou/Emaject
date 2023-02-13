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
            std::cout << "[hello world]" << str << std::endl;
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

    TEST_CASE("hello world")
    {
        Injector injector;
        injector.install<CoutInstaller>();

        auto printer = injector.resolve<IPrinter>();
        printer->println("Hello World");
    }
}