#include <Emaject.hpp>

#include <iostream>
#include <string_view>
#include "catch.hpp"

namespace
{
    using emaject::Container;
    using emaject::IInstaller;
    using emaject::Injector;

    class Hoge
    {
    public:
        Hoge(int v):
            value(v)
        {}
        int value;
    };

    class Foo
    {
    public:
        Foo(int v) :
            value(v)
        {}
        int value;
    };

    class HogeFoo
    {
    public:
        int value()
        {
            return m_hoge->value + m_foo->value;
        }
    private:
        [[INJECT(setHogeAndFoo)]]
        void setHogeAndFoo(
            const std::shared_ptr<Hoge>& hoge,
            const std::shared_ptr<Foo>& foo
        ) {
            m_hoge = hoge;
            m_foo = foo;
        }
    private:
        std::shared_ptr<Hoge> m_hoge;
        std::shared_ptr<Foo> m_foo;
    };
    struct HogeFooInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<Hoge>().withArgs(10).asTransient();
            c->bind<Foo>().withArgs(200).asTransient();
        }
    };

    TEST_CASE("method_inject")
    {
        Injector injector;
        injector.install<HogeFooInstaller>();

        auto hogeFoo = injector.instantiate<HogeFoo>();
        REQUIRE(hogeFoo->value() == 210);
    }
}
