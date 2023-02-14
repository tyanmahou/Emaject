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
        int value() const
        {
            return m_hoge->value + m_foo->value;
        }
        int value2() const
        {
            return m_hoge2->value + m_foo2->value;
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
        [[INJECT(setHogeAndFoo2, 1, 2)]]
        void setHogeAndFoo2(
            const std::shared_ptr<Hoge>& hoge,
            const std::shared_ptr<Foo>& foo
        ) {
            m_hoge2 = hoge;
            m_foo2 = foo;
        }
    private:
        std::shared_ptr<Hoge> m_hoge;
        std::shared_ptr<Foo> m_foo;

        std::shared_ptr<Hoge> m_hoge2;
        std::shared_ptr<Foo> m_foo2;
    };
    struct HogeFooInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<Hoge>().withArgs(20).asTransient();
            c->bind<Foo>().withArgs(100).asTransient();

            c->bind<Hoge, 1>().withArgs(10).asTransient();
            c->bind<Foo, 2>().withArgs(200).asTransient();
        }
    };

    TEST_CASE("method_inject")
    {
        Injector injector;
        injector.install<HogeFooInstaller>();

        auto hogeFoo = injector.instantiate<HogeFoo>();
        REQUIRE(hogeFoo->value() == 120);
        REQUIRE(hogeFoo->value2() == 210);
    }
}
