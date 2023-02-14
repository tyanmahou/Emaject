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
    };
    class IFooFoo : public IFoo
    {
    };
    class FooFoo : public IFooFoo
    {
    };
    struct FooInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<IFooFoo>()
                .to<FooFoo>()
                .asCached();

            c->bind<IFoo>()
                .to<IFooFoo>()
                .fromResolve()
                .asCached();
        }
    };
    TEST_CASE("from_resolve")
    {
        Injector injector;
        injector.install<FooInstaller>();

        {
            auto foo = injector.resolve<IFoo>();
            auto foofoo = injector.resolve<IFooFoo>();

            REQUIRE(foo != nullptr);
            REQUIRE(foo == foofoo);
        }
    }
}