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
            std::cout << "[lambda_install]" << str << std::endl;
        }
    };

    TEST_CASE("lambda_install")
    {
        Injector injector;
        injector.install([](Container* c) {
            c->bind<IPrinter>()
            .to<CoutPrinter>()
            .asCache();
            });

        auto printer = injector.resolve<IPrinter>();
        printer->println("Hello World");
    }
}
