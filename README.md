# Emaject
C++20 DI Library

## About

"Emaject" is a `Dependency Injection` library for C++20 only with a header;

## How to

Header Include Only `Emaject.hpp`

## Example

```cpp
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

int main()
{
    Injector injector;
    injector.install<CoutInstaller>();

    auto printer = injector.resolve<IPrinter>();
    printer->println("Hello World");
}
```
### Field Inject

This is executed after the constructor.

```cpp
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
```

### Constructor Inject

```cpp
class HelloWorld
{
public:
    INJECT_CTOR(HelloWorld(std::shared_ptr<IPrinter> printer)):
        m_printer(printer)
    {}

    void greet() const
    {
        m_printer->println("Hello World");
    }
private:
    std::shared_ptr<IPrinter> m_printer;
};
```
### Traits Inject

You can also inject using `InjectTraits`.
This is executed after the constructor.

But. **Field Inject** also uses `InjectTraits`, so be careful of conflicts!!

```cpp
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
```

### ID

You can use an ID for binding. If not specified, it will be `0`.

```cpp
class PrintfPrinter : public IPrinter
{
public:
    void println(std::string_view str) const override
    {
        std::printf("%s for printf", str.data());
    }
};

struct PrintfInstaller : IInstaller
{
    void onBinding(Container* c) const
    {
        // ID = 1 -> PrintfPrinter
        c->bind<IPrinter, 1>()
            .to<PrintfPrinter>()
            .asCache();
    }
};
```
You can specify the ID with field inject.

```cpp
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
    [[INJECT(m_printer0)]] // default ID = 0
    std::shared_ptr<IPrinter> m_printer0;

    [[INJECT(m_printer1, 1)]] // ID = 1
    std::shared_ptr<IPrinter> m_printer1;
};
```