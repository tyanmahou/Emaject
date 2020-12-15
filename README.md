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

⚠️__But. *Field Inject* also uses `InjectTraits`, so be careful of conflicts!!__

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
or resolve

```cpp
container->resolve<Type, ID>();
```

### Scope

#### Transient

If you use `asTransient()`, an instance will be created for each injection.

```cpp
struct CounterInstaller : IInstaller
{
    void onBinding(Container* c) const
    {
        c->bind<ICounter>()
            .to<Counter>()
            .asTransient();
    }
};

int main()
{
    Injector injector;
    injector.install<CounterInstaller>();

    {
        auto counter = injector.resolve<ICounter>();  // new instance
        std::cout << counter->countUp() << std::endl; // 1
    }
    {
        auto counter = injector.resolve<ICounter>();  // new instance
        std::cout << counter->countUp() << std::endl; // 1 
    }
}
```

#### Cache

If you use `asCache`, the one cached by the ID will be reused.

```cpp
struct CounterInstaller : IInstaller
{
    void onBinding(Container* c) const
    {
        container->bind<ICounter>()
            .to<Counter>()
            .asCache();
    }
};

int main()
{
    Injector injector;
    injector.install<CounterInstaller>();

    {
        auto counter = injector.resolve<ICounter>();  // new instance
        std::cout << counter->countUp() << std::endl; // 1
    }
    {
        auto counter = injector.resolve<ICounter>();  // use cache 
        std::cout << counter->countUp() << std::endl; // 2 
    }
}
```

#### Single

If you use `asSingle`, the cached one will be reused regardless of the ID.
ID cannot be used in binding that used this.

```cpp
struct CounterInstaller : IInstaller
{
    void onBinding(Container* c) const
    {
        c->bind<ICounter>()
            .to<Counter>()
            .asSingle();

        // compile error "don't use ID"
        //c->bind<ICounter, 1>()
        //    .to<Counter>()
        //    .asSingle();
    }
};

int main()
{
    Injector injector;
    injector.install<CounterInstaller>();

    {
        auto counter = injector.resolve<ICounter, 0>(); // new instance
        std::cout << counter->countUp() << std::endl;   // 1
    }
    {
        auto counter = injector.resolve<ICounter, 1>(); // used cache
        std::cout << counter->countUp() << std::endl;   // 2 
    }
}
```