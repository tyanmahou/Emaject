#pragma once

#include <memory>
#include <functional>
#include <any>
#include <unordered_map>
#include <typeindex>

namespace emaject
{
    class Container;

    template<class Type>
    struct InjectTraits
    {
        //void onInject(Type* value, Container* c)
    };

    template<class Type>
    concept FieldInjectable = requires(Type * t, Container * c)
    {
        InjectTraits<Type>{}.onInject(t, c);
    };
    template<class Type>
    concept CtorInjectable = requires()
    {
        typename Type::CtorInject;
    };

    /// <summary>
    /// Container
    /// </summary>
    class Container
    {
        enum class ScopeKind
        {
            Transient,
            Cache,
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
        template<class Type>
        struct Builder
        {
            template<class T>
            struct ctor {};
            template<class T, class... Args>
            struct ctor<T(Args...)>
            {
                auto operator()(Container* c)
                {
                    return  std::make_shared<Type>(c->resolve<typename std::decay_t<Args>::element_type>()...);
                }
            };

            auto operator()(Container* c)
            {
                std::shared_ptr<Type> ret;
                if constexpr (CtorInjectable<Type>) {
                    ret = ctor<typename Type::CtorInject>{}(c);
                } else {
                    ret = std::make_shared<Type>();
                }
                if constexpr (FieldInjectable<Type>) {
                    if (ret) {
                        InjectTraits<Type>{}.onInject(ret.get(), c);
                    }
                }
                return ret;
            }
        };

        template<class Type>
        std::shared_ptr<Type> build()
        {
            if constexpr (!std::is_abstract_v<Type>) {
                return Builder<Type>{}(this);
            } else {
                return nullptr;
            }
        }
        template<class Type, int ID>
        class ScopeRegister
        {
        private:
            Container* m_container;
            Factory<Type> m_factory;
        public:
            ScopeRegister(Container* c, const Factory<Type>& f) :
                m_container(c),
                m_factory(f)
            {}
            bool asTransient() const
            {
                return m_container
                    ->bindRegist<Type, ID>(BindInfo<Type>{m_factory, ScopeKind::Transient, nullptr});
            }
            bool asCache() const
            {
                return m_container
                    ->bindRegist<Type, ID>(BindInfo<Type>{m_factory, ScopeKind::Cache, nullptr });
            }
            bool asSingle() const requires (ID == 0)
            {
                return m_container
                    ->bindRegist<Type, ID>(BindInfo<Type>{m_factory, ScopeKind::Single, nullptr });
            }
        };
        template<class Type, int ID>
        class Binder
        {
        private:
            Container* m_container;
        public:
            Binder(Container* c) :
                m_container(c)
            {}

            template<class U>
            [[nodiscard]] auto to() const
            {
                return fromFactory([c = m_container]() {
                    return c->build<U>();
                });
            }
            [[nodiscard]] auto toSelf() const
            {
                return to<Type>();
            }
            [[nodiscard]] auto fromInstance(const std::shared_ptr<Type>& instance) const
            {
                return fromFactory([i = instance]() {
                    return i;
                });
            }
            [[nodiscard]] auto fromFactory(const Factory<Type>& factory) const
            {
                return ScopeRegister<Type, ID>(m_container, factory);
            }
            bool asTransient() const
            {
                return toSelf().asTransient();
            }
            bool asCache() const
            {
                return toSelf().asCache();
            }
            bool asSingle() const
            {
                return toSelf().asSingle();
            }
        };
        template<class Type, int ID>
        bool bindRegist(const BindInfo<Type>& info)
        {
            const auto& id = info.kind == ScopeKind::Single ? typeid(Type) :typeid(Tag<Type, ID>);
            if (m_bindInfos.find(id) != m_bindInfos.end()) {
                return false;
            }
            m_bindInfos[id] = info;
            return true;
        }

        template<class Type, int ID = 0>
        const std::type_info& resolveId() const
        {
            const auto& singleId = typeid(Type);
            if (m_bindInfos.find(singleId) != m_bindInfos.end()) {
                return singleId;
            }
            return typeid(Tag<Type, ID>);
        }
    public:
        template<class Type, int ID = 0>
        [[nodiscard]] auto bind()
        {
            return Binder<Type, ID>(this);
        }

        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            const auto& id = resolveId<Type, ID>();
            if (m_bindInfos.find(id) == m_bindInfos.end()) {
                return this->build<Type>();
            }
            auto&& [factory, createKind, cache] = std::any_cast<BindInfo<Type>&>(m_bindInfos.at(id));
            if (createKind != ScopeKind::Transient) {
                if (!cache) {
                    cache = factory();
                }
                return std::static_pointer_cast<Type>(cache);
            }
            return std::static_pointer_cast<Type>(factory());
        }
    private:
        std::unordered_map<std::type_index, std::any> m_bindInfos;
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
    private:
        std::shared_ptr<Container> m_container;
    public:
        Injector():
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
        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            return m_container->resolve<Type, ID>();
        }
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
        void auto_inject_all_impl([[maybe_unused]] Type& ret, Container* c, std::index_sequence<Seq...>)
        {
            (auto_inject<Type, Seq>(ret, c), ...);
        }
        template<IsAutoInjectable Type>
        void auto_inject_all(Type& ret, Container* c)
        {
            auto_inject_all_impl(ret, c, make_sequence<Type>());
        }
    }

    template<detail::IsAutoInjectable Type>
    struct InjectTraits<Type>
    {
        void onInject(Type* value, Container* c)
        {
            detail::auto_inject_all(*value, c);
        }
    };
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
