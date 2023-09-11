#pragma once

#include <memory>
#include <functional>
#include <any>
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace emaject
{
    class Container;

    template<class Type>
    struct InjectTraits
    {
        //void onInject(Type* value, Container* c)
    };

    namespace detail
    {
        template<class Type>
        struct AutoInjector;

        template<auto Method, int... IDs>
        struct MethodInjector;

        template<class Type>
        concept TraitsInjectable = requires(Type * t, Container * c)
        {
            InjectTraits<Type>{}.onInject(t, c);
        };

        template<class Type>
        concept AutoInjectable = requires(Type * t, Container * c)
        {
            AutoInjector<Type>{}.onInject(t, c);
        };

        template<class Type, auto Method>
        concept MethodInjectable = requires(Type * t, Container * c)
        {
            requires std::is_member_function_pointer_v<decltype(Method)>;
            typename MethodInjector<Method>::Type;
            MethodInjector<Method>{}.onInject(t, c);
        };

        template<class Type>
        concept CtorInjectable = requires()
        {
            typename Type::CtorInject;
        };

        template <class Type, class... Args>
        concept DefaultInstantiatable = (sizeof...(Args) == 0 && detail::CtorInjectable<Type>) || std::constructible_from<Type, Args...>;
    }

    /// <summary>
    /// Container
    /// </summary>
    class Container
    {
    public:
        template<class Type, int ID = 0>
        [[nodiscard]] auto bind()
        {
            return Binder<Type, ID>(this);
        }

        template<class Type, class...Args>
        [[nodiscard]] std::shared_ptr<Type> instantiate(Args&&... args)
        {
            auto ret = this->makeInstance<Type>(std::forward<Args>(args)...);
            this->inject(ret.get());
            return ret;
        }
        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            auto itr = m_bindInfos.find(typeid(Tag<Type, ID>));
            if (itr == m_bindInfos.end()) {
                return nullptr;
            }
            auto&& [factory, createKind, cache] = std::any_cast<BindInfo<Type>&>(itr->second);
            if (createKind != ScopeKind::Transient) {
                if (!cache) {
                    cache = factory(this);
                }
                return std::static_pointer_cast<Type>(cache);
            }
            return std::static_pointer_cast<Type>(factory(this));
        }

        template<class Type>
        void inject(Type* value)
        {
            if constexpr (detail::AutoInjectable<Type>) {
                if (value) {
                    detail::AutoInjector<Type>{}.onInject(value, this);
                }
            }
            if constexpr (detail::TraitsInjectable<Type>) {
                if (value) {
                    InjectTraits<Type>{}.onInject(value, this);
                }
            }
        }
    private:
        enum class ScopeKind
        {
            Transient,
            Cached,
            Single
        };
        template<class Type, int ID>
        struct Tag {};

        template<class Type>
        using Factory = std::function<std::shared_ptr<Type>(Container*)>;

        template<class Type>
        struct BindInfo
        {
            Factory<Type> factory;
            ScopeKind kind;
            std::shared_ptr<Type> cache;
        };
        struct DerefInfo
        {
            bool isSingle = false;
            std::vector<std::type_index> bindIds;
        };
        template<class Type, int ID>
        class Binder
        {
        public:
            Binder(Container* c) :
                m_container(c)
            {}
        public:
            template<class To>
            [[nodiscard]] auto to() const
            {
                return FromDescriptor<Type, To, ID>(m_container);
            }
            [[nodiscard]] auto toSelf() const
            {
                return FromDescriptor<Type, Type, ID>(m_container);
            }
        public:
            [[nodiscard]] auto fromNew() const requires detail::DefaultInstantiatable<Type>
            {
                return toSelf().fromNew();
            }
            template<class... Args>
            [[nodiscard]] auto withArgs(Args&&... args) const requires std::constructible_from<Type, Args...>
            {
                return toSelf().withArgs(std::forward<Args>(args)...);
            }
            [[nodiscard]] auto fromInstance(const std::shared_ptr<Type>& instance) const
            {
                return toSelf().fromInstance(instance);
            }
            [[nodiscard]] auto fromFactory(const Factory<Type>& factory) const
            {
                return toSelf().fromFactory(factory);
            }
            [[nodiscard]] auto fromFactory(const std::function<std::shared_ptr<Type>()>& factory) const
            {
                return toSelf().fromFactory(factory);
            }
            [[nodiscard]] auto unused() const
            {
                return toSelf().unused();
            }
        public:
            bool asTransient() const requires detail::DefaultInstantiatable<Type>
            {
                return fromNew().asTransient();
            }
            bool asCached() const requires detail::DefaultInstantiatable<Type>
            {
                return fromNew().asCached();
            }
            bool asSingle() const requires detail::DefaultInstantiatable<Type>
            {
                return fromNew().asSingle();
            }
        private:
            Container* m_container;
        };

        template<class From, class To, int ID>
        class FromDescriptor
        {
        public:
            FromDescriptor(Container* c) :
                m_container(c)
            {}
        public:
            [[nodiscard]] auto fromNew() const requires detail::DefaultInstantiatable<To>
            {
                return fromFactory([] (Container* c){
                    return c->instantiate<To>();
                });
            }
            template<class... Args>
            [[nodiscard]] auto withArgs(Args&&... args) const requires std::constructible_from<To, Args...>
            {
                return fromFactory([...args = std::forward<Args>(args)](Container* c){
                    // not allow move becouse transient
                    auto ret = std::make_shared<To>(args...);
                    c->inject(ret.get());
                    return ret;
                });
            }
            [[nodiscard]] auto fromInstance(const std::shared_ptr<To>& instance) const
            {
                return fromFactory([i = instance] {
                    return i;
                });
            }
            [[nodiscard]] auto fromFactory(const Factory<To>& factory) const
            {
                return ScopeDescriptor<From, To, ID>(m_container, factory);
            }
            [[nodiscard]] auto fromFactory(const std::function<std::shared_ptr<To>()>& factory) const
            {
                return fromFactory([factory]([[maybe_unused]] Container* container) {
                    return factory();
                });
            }
            template<int RsolveID = 0>
            [[nodiscard]] auto fromResolve() const
            {
                return fromFactory([](Container* c) {
                    return c->resolve<To, RsolveID>();
                });
            }
            [[nodiscard]] auto unused() const
            {
                return fromFactory([]{
                    return nullptr;
                });
            }
        public:
            bool asTransient() const requires detail::DefaultInstantiatable<To>
            {
                return fromNew().asTransient();
            }
            bool asCached() const requires detail::DefaultInstantiatable<To>
            {
                return fromNew().asCached();
            }
            bool asSingle() const requires detail::DefaultInstantiatable<To>
            {
                return fromNew().asSingle();
            }
        private:
            Container* m_container;
        };
        template<class From, class To, int ID>
        class ScopeDescriptor
        {
        public:
            ScopeDescriptor(Container* c, const Factory<To>& f) :
                m_container(c),
                m_factory(f)
            {}
        public:
            bool asTransient() const
            {
                return m_container
                    ->regist<From, To, ID>({ m_factory, ScopeKind::Transient, nullptr });
            }
            bool asCached() const
            {
                return m_container
                    ->regist<From, To, ID>({ m_factory, ScopeKind::Cached, nullptr });
            }
            bool asSingle() const
            {
                return m_container
                    ->regist<From, To, ID>({ m_factory, ScopeKind::Single, nullptr });
            }
        private:
            Container* m_container;
            Factory<To> m_factory;
        };

        template<class Type>
        struct Instantiater
        {
            template<class T>
            struct ctor {};
            template<class T, class... Args>
            struct ctor<T(Args...)>
            {
                auto operator()(Container* c) const
                {
                    return std::make_shared<Type>(c->resolve<typename std::decay_t<Args>::element_type>()...);
                }
            };

            template<class... Args>
            auto operator()(Container* c, Args&&... args) const
            {
                if constexpr (sizeof...(Args) == 0 && detail::CtorInjectable<Type>) {
                    return ctor<typename Type::CtorInject>{}(c);
                } else if constexpr (std::constructible_from<Type, Args...>) {
                    return std::make_shared<Type>(std::forward<Args>(args)...);
                } else {
                    return nullptr;
                }
            }
        };
    private:
        template<class Type, class...Args>
        [[nodiscard]] std::shared_ptr<Type> makeInstance(Args&&... args)
        {
            return Instantiater<Type>{}(this, std::forward<Args>(args)...);
        }

        template<class From, class To, int ID>
        bool regist(const BindInfo<From>& info)
        {
            auto& deref = m_derefInfos[typeid(To)];
            if (deref.isSingle) {
                return false;
            }
            const auto& id = typeid(Tag<From, ID>);
            if (m_bindInfos.find(id) != m_bindInfos.end()) {
                return false;
            }
            if (info.kind == ScopeKind::Single) {
                if (!deref.bindIds.empty()) {
                    return false;
                }
                deref.isSingle = true;
            }
            m_bindInfos[id] = info;
            deref.bindIds.push_back(id);
            return true;
        }
    private:
        std::unordered_map<std::type_index, std::any> m_bindInfos;
        std::unordered_map<std::type_index, DerefInfo> m_derefInfos;
    };

    /// <summary>
    /// Installer
    /// </summary>
    struct IInstaller
    {
        virtual ~IInstaller() = default;
        virtual void onBinding(Container* c) const = 0;
    };

    /// <summary>
    /// Injector
    /// </summary>
    class Injector
    {
    public:
        Injector() :
            m_container(std::make_shared<Container>())
        {}

        template<class Installer, class...Args>
        Injector& install(Args&&... args) requires std::is_base_of_v<IInstaller, Installer>
        {
            Installer(std::forward<Args>(args)...).onBinding(m_container.get());
            return *this;
        }

        Injector& install(const std::function<void(Container* c)>& installer)
        {
            installer(m_container.get());
            return *this;
        }

        template<class Type, class... Args>
        [[nodiscard]] std::shared_ptr<Type> instantiate(Args&&... args) requires detail::DefaultInstantiatable<Type, Args...>
        {
            return m_container->instantiate<Type>(std::forward<Args>(args)...);
        }
        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            return m_container->resolve<Type, ID>();
        }
    private:
        std::shared_ptr<Container> m_container;
    };


    //----------------------------------------
    // Auto Injector
    //----------------------------------------
    namespace detail
    {
#if __clang__
        inline constexpr size_t AUTO_INJECT_MAX_LINES = 256;
#else
        inline constexpr size_t AUTO_INJECT_MAX_LINES = 500;
#endif
        template<size_t Line>
        struct AutoInjectLine
        {
            Container* container;
        };

        template<class Type, size_t LineNum>
        concept AutoInjectCallable = requires(Type t, AutoInjectLine<LineNum> l)
        {
            t | l;
        };

        template <size_t... As, size_t... Bs>
        constexpr std::index_sequence<As..., Bs...> operator+(std::index_sequence<As...>, std::index_sequence<Bs...>)
        {
            return {};
        }
        template <class Type, size_t LineNum>
        constexpr auto filter_seq()
        {
            if constexpr (AutoInjectCallable<Type, LineNum>) {
                return std::index_sequence<LineNum>{};
            } else {
                return std::index_sequence<>{};
            }
        }
        template <class Type, size_t ...Seq>
        constexpr auto make_sequence_impl(std::index_sequence<Seq...>)
        {
            return (filter_seq<Type, Seq>() + ...);
        }

        template <class Type>
        constexpr auto make_sequence()
        {
            return make_sequence_impl<Type>(std::make_index_sequence<AUTO_INJECT_MAX_LINES>());
        }

        template<class Type>
        concept IsAutoInjectable = decltype(make_sequence<Type>())::size() > 0;

        template<IsAutoInjectable Type, size_t LineNum>
        void auto_inject(Type& ret, Container* c)
        {
            ret | AutoInjectLine<LineNum>{c};
        }
        template<IsAutoInjectable Type, size_t ...Seq>
        void auto_inject_all_impl(Type& ret, Container* c, std::index_sequence<Seq...>)
        {
            (auto_inject<Type, Seq>(ret, c), ...);
        }
        template<IsAutoInjectable Type>
        void auto_inject_all(Type& ret, Container* c)
        {
            auto_inject_all_impl(ret, c, make_sequence<Type>());
        }

        template<class Type>
        struct AutoInjector
        {};

        template<IsAutoInjectable Type>
        struct AutoInjector<Type>
        {
            void onInject(Type* value, Container* c)
            {
                detail::auto_inject_all(*value, c);
            }
        };

        template<class T, auto MemP>
        struct InjectedType{};

        template<class Type, class R, R Type::* MemP>
        struct InjectedType<R Type::*, MemP>
        {
            using type = Type;
        };

        template<class Type, class R, class... Args, R(Type::* MemP)(Args...)>
        struct InjectedType<R(Type::*)(Args...), MemP>
        {
            using type = Type;
        };

        template<auto MemP, int... IDs>
        struct MethodInjector
        {
            template<class T, T Method>
            struct impl
            {
            };
            template<class Type, class R, class... Args, R(Type::* Method)(Args...)>
            struct impl<R(Type::*)(Args...), Method>
            {
                using resolves_type = std::tuple<Args...>;

                template<int ID>
                struct IDType
                {
                    static constexpr int id = ID;
                };
                using resolves_id = std::tuple<IDType<IDs>...>;

                template<size_t Index>
                struct TypeId
                {
                    using type = typename std::decay_t<std::tuple_element_t<Index, resolves_type>>::element_type;
                    static consteval int id()
                    {
                        if constexpr (Index < sizeof...(IDs)) {
                            return std::tuple_element_t<Index, resolves_id>::id;
                        } else {
                            return 0;
                        }
                    }
                };
                template<size_t Index>
                auto resolve(Container* c)
                {
                    return c->resolve<typename TypeId<Index>::type, TypeId<Index>::id()>();
                }
                template<size_t... Seq>
                auto onInject(Type* value, Container* c, std::index_sequence<Seq...>)
                {
                    return (value->*MemP)(resolve<Seq>(c)...);
                }
                auto onInject(Type* value, Container* c)
                {
                    return onInject(value, c, std::make_index_sequence<sizeof...(Args)>());
                }
            };
            using Type = typename InjectedType<decltype(MemP), MemP>::type;

            auto onInject(Type* t, Container* c)
            {
                return impl<decltype(MemP), MemP>{}.onInject(t, c);
            }
        };
        template<auto MemP, int ID>
        struct FieldInjector
        {
            using Type = typename InjectedType<decltype(MemP), MemP>::type;

            void onInject(Type* t, Container* c)
            {
                t->*MemP = c->resolve<typename std::decay_t<decltype(t->*MemP)>::element_type, ID>();
            }
        };
        template<class Type, auto MemP, int ID = 0, int... IDs>
        void AutoInject(Type* value, Container* c)
        {
            if constexpr (::emaject::detail::MethodInjectable<Type, MemP>) {
                MethodInjector<MemP, ID, IDs...>{}.onInject(value, c);
            } else {
                FieldInjector<MemP, ID>{}.onInject(value, c);
            }
        }
    }
}

//----------------------------------------
// Macro
//----------------------------------------
#if _MSC_VER
#define INJECT_IMPL(value, line, ...) ]]\
friend auto operator|(auto& a, const ::emaject::detail::AutoInjectLine<line>& l){\
    static_assert(line < ::emaject::detail::AUTO_INJECT_MAX_LINES);\
    using ThisType = std::decay_t<decltype(a)>;\
    ::emaject::detail::AutoInject<ThisType, &ThisType::value, __VA_ARGS__>(&a, l.container);\
}[[
#else
#define INJECT_IMPL(value, line, ...) ]]\
friend auto operator|(auto& a, const ::emaject::detail::AutoInjectLine<line>& l){\
    static_assert(line < ::emaject::detail::AUTO_INJECT_MAX_LINES);\
    using ThisType = std::decay_t<decltype(a)>;\
    ::emaject::detail::AutoInject<ThisType, &ThisType::value __VA_OPT__(,) __VA_ARGS__>(&a, l.container);\
}[[
#endif
#define INJECT(value, ...) INJECT_IMPL(value, __LINE__, __VA_ARGS__)

#define INJECT_CTOR(ctor) using CtorInject = ctor; ctor
