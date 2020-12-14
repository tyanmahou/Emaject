#include <Emaject.hpp>

#include <iostream>
#include <string_view>

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
    HelloWorld() = default;

    void greet() const
    {
        m_printer->println("Hello World");
    }
private:
    [[INJECT(m_printer)]]
    std::shared_ptr<IPrinter> m_printer;
};

int main()
{
    Injector injector;
    injector.install<CoutInstaller>();

    auto helloWorld = injector.resolve<HelloWorld>();
    helloWorld->greet();
}