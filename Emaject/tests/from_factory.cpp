#include <Emaject.hpp>

#include <string_view>
#include "catch.hpp"

namespace
{
    using emaject::Container;
    using emaject::IInstaller;
    using emaject::Injector;

    class IFoo
    {
    public:
        virtual ~IFoo() = default;
        virtual int value() const = 0;
    };
    class Foo : public IFoo
    {
    public:
        Foo(int value) :
            m_value(value)
        {
        }
        int value() const override
        {
            return m_value;
        }
    private:
        int m_value;
    };

    struct FooInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            // No arg Factory
            c->bind<IFoo, 0>()
                .toSelf()
                .fromFactory([] {
                    return std::make_shared<Foo>(1);
                 })
                .asCached();

            // Container arg Factory
            c->bind<IFoo, 1>()
                .toSelf()
                .fromFactory([](Container* c) {
                    return std::make_shared<Foo>(c->resolve<IFoo, 0>()->value() + 1);
                })
                .asCached();
        }
    };
    TEST_CASE("from_factory")
    {
        Injector injector;
        injector.install<FooInstaller>();

        {
            auto foo0 = injector.resolve<IFoo, 0>();
            auto foo1 = injector.resolve<IFoo, 1>();
            REQUIRE(foo0->value() == 1);
            REQUIRE(foo1->value() == 2);
        }
    }
}