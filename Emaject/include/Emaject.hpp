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
        struct FieldInjector {};

        template<class Type>
        concept TraitsInjectable = requires(Type * t, Container * c)
        {
            InjectTraits<Type>{}.onInject(t, c);
        };

        template<class Type>
        concept FieldInjectable = requires(Type * t, Container * c)
        {
            FieldInjector<Type>{}.onInject(t, c);
        };
        template<class Type>
        concept CtorInjectable = requires()
        {
            typename Type::CtorInject;
        };

        template <class Type>
        concept DefaultInstantiatable = detail::CtorInjectable<Type> || std::default_initializable<Type>;
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

        template<class Type>
        [[nodiscard]] std::shared_ptr<Type> instantiate()
        {
            auto ret = this->makeInstance<Type>();
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
                    cache = factory();
                }
                return std::static_pointer_cast<Type>(cache);
            }
            return std::static_pointer_cast<Type>(factory());
        }

        template<class Type>
        void inject(Type* value)
        {
            if constexpr (detail::FieldInjectable<Type>) {
                if (value) {
                    detail::FieldInjector<Type>{}.onInject(value, this);
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
        using Factory = std::function<std::shared_ptr<Type>()>;

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
                return fromFactory([c = m_container] {
                    return c->instantiate<To>();
                });
            }
            template<class... Args>
            [[nodiscard]] auto withArgs(Args&&... args) const requires std::constructible_from<To, Args...>
            {
                return fromFactory([c = m_container, ...args = std::forward<Args>(args)] {
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
            [[nodiscard]] auto unused() const
            {
                return fromFactory([] {
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
                    return std::make_shared<T>(c->resolve<typename std::decay_t<Args>::element_type>()...);
                }
            };

            auto operator()(Container* c) const
            {
                if constexpr (detail::CtorInjectable<Type>) {
                    return ctor<typename Type::CtorInject>{}(c);
                } else if constexpr (std::default_initializable<Type>) {
                    return std::make_shared<Type>();
                } else {
                    return nullptr;
                }
            }
        };
    private:
        template<class Type>
        [[nodiscard]] std::shared_ptr<Type> makeInstance()
        {
            return Instantiater<Type>{}(this);
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
            m_bindInfos[id] = info;
            if (info.kind == ScopeKind::Single) {
                // remove duplicated bind
                for (const auto& bindId : deref.bindIds) {
                    m_bindInfos.erase(bindId);
                }
                deref.bindIds.clear();
                deref.isSingle = true;
            }
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

        template<class Type>
        [[nodiscard]] std::shared_ptr<Type> instantiate() requires detail::DefaultInstantiatable<Type>
        {
            return m_container->instantiate<Type>();
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
        inline constexpr size_t AUTO_INJECT_MAX_LINES = 500;

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

        template<IsAutoInjectable Type>
        struct FieldInjector<Type>
        {
            void onInject(Type* value, Container* c)
            {
                detail::auto_inject_all(*value, c);
            }
        };
    }
}

//----------------------------------------
// Macro
//----------------------------------------

#define INJECT_PP_IMPL_OVERLOAD(e1, e2, NAME, ...) NAME
#define INJECT_PP_IMPL_2(value, id) ]];\
template<class ThisType>\
friend auto operator|(ThisType& a, const ::emaject::detail::AutoInjectLine<__LINE__>& l){\
    static_assert(__LINE__ < ::emaject::detail::AUTO_INJECT_MAX_LINES);\
    a.value = l.container->resolve<typename decltype(a.value)::element_type, id>();\
}[[
#define INJECT_PP_IMPL_1(value) INJECT_PP_IMPL_2(value, 0)
#define INJECT_PP_EXPAND( x ) x
#define INJECT(...) INJECT_PP_EXPAND(INJECT_PP_IMPL_OVERLOAD(__VA_ARGS__, INJECT_PP_IMPL_2, INJECT_PP_IMPL_1)(__VA_ARGS__))

#define INJECT_CTOR(ctor) using CtorInject = ctor; ctor
