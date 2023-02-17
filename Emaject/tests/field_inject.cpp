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
        virtual ~IPrinter() = default;
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
                .asCached();
        }
    };

    class HelloWorld
    {
    public:
        HelloWorld() = default;

        void greet() const
        {
            m_printer->println("[field_inject] Hello World");
        }
    private:
        [[INJECT(m_printer)]]
        std::shared_ptr<IPrinter> m_printer;
    };

    TEST_CASE("field_inject")
    {
        static_assert(emaject::detail::IsAutoInjectable<HelloWorld>);
        Injector injector;
        injector.install<CoutInstaller>();

        auto helloWorld = injector.instantiate<HelloWorld>();
        helloWorld->greet();
    }
}
