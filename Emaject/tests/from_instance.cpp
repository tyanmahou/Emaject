#include <Emaject.hpp>
#include "catch.hpp"

namespace
{
    using emaject::Container;
    using emaject::IInstaller;
    using emaject::Injector;


    class Singleton
    {
    public:
        static std::shared_ptr<Singleton> Instance()
        {
            static auto instance = std::shared_ptr<Singleton>(new Singleton());
            return instance;
        }
    private:
        Singleton() = default;
        Singleton(const Singleton& other) = delete;
        Singleton& operator=(const Singleton& other) = delete;
    };
    struct SingletonInstaller : IInstaller
    {
        void onBinding(Container* c) const
        {
            c->bind<Singleton>()
                .toSelf()
                .fromInstance(Singleton::Instance())
                .asSingle();
        }
    };
    TEST_CASE("from_instance")
    {
        Injector injector;
        injector.install<SingletonInstaller>();
        {
            auto singleton = injector.resolve<Singleton>();
            REQUIRE(Singleton::Instance().get() == singleton.get());
        }
    }
}