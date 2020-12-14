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
    class Container
    {
        enum class CreateKind
        {
            Transient,
            Cache,
        };
        template<class Type, int ID>
        struct Tag {};

        template<class Type>
        using CreateFunc = std::function<std::shared_ptr<Type>()>;

        template<class Type>
        struct BindInfo
        {
            CreateFunc<Type> func;
            CreateKind kind;
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
        class BindRegister
        {
        private:
            Container* m_container;
            CreateFunc<Type> m_func;
        public:
            BindRegister(Container* c, const CreateFunc<Type>& f) :
                m_container(c),
                m_func(f)
            {}
            bool asTransient() const
            {
                return m_container
                    ->bindRegist<Type, ID>(BindInfo<Type>{m_func, CreateKind::Transient});
            }
            bool asCache() const
            {
                return m_container
                    ->bindRegist<Type, ID>(BindInfo<Type>{m_func, CreateKind::Cache});
            }
        };
        template<class Type, int ID>
        class Bainder
        {
        private:
            Container* m_container;
        public:
            Bainder(Container* c) :
                m_container(c)
            {}

            template<class U>
            [[nodiscard]] auto to()
            {
                return fromInstance([c = m_container]() {
                    return c->build<U>();
                });
            }
            [[nodiscard]] auto fromInstance(const CreateFunc<Type>& func)
            {
                return BindRegister<Type, ID>(m_container, func);
            }
        };
        template<class Type, int ID>
        bool bindRegist(const BindInfo<Type>& info)
        {
            const auto& id = typeid(Tag<Type, ID>);
            if (m_createFuncs.find(id) != m_createFuncs.end()) {
                return false;
            }
            m_createFuncs[id] = info;
            return true;
        }
    public:
        template<class Type, int ID = 0>
        [[nodiscard]] auto bind()
        {
            return Bainder<Type, ID>(this);
        }

        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            const auto& id = typeid(Tag<Type, ID>);
            if (m_instanceCache.find(id) != m_instanceCache.end()) {
                return std::any_cast<std::shared_ptr<Type>>(m_instanceCache[id]);
            }
            if (m_createFuncs.find(id) == m_createFuncs.end()) {
                return this->build<Type>();
            }
            auto [func, createKind] = std::any_cast<BindInfo<Type>>(m_createFuncs.at(id));
            auto ret = func();
            if (createKind == CreateKind::Cache) {
                m_instanceCache[id] = ret;
            }
            return ret;
        }
    private:
        std::unordered_map<std::type_index, std::any> m_createFuncs;
        std::unordered_map<std::type_index, std::any> m_instanceCache;
    };

    struct IInstaller
    {
        virtual ~IInstaller() = default;
        virtual void onBinding(Container* c) const = 0;
    };


    class Injector
    {
    private:
        Container m_container;
    public:

        template<class Installer, class...Args>
        Injector& install(Args&&... args)
        {
            Installer(std::forward<Args>(args)...).onBinding(&m_container);
            return *this;
        }
        template<class Type, int ID = 0>
        [[nodiscard]] std::shared_ptr<Type> resolve()
        {
            return m_container.resolve<Type, ID>();
        }
    };


    //----------------------------------------
    // Auto Injector
    //----------------------------------------
    namespace detail
    {
        inline constexpr int AUTO_INJECT_MAX_LINES = 500;

        template<int Line>
        struct AutoInjectLine
        {
            Container* container;
        };

        template<class Type, int LineNum>
        concept AutoInjectCallable = requires(Type t, AutoInjectLine<LineNum> l)
        {
            t | l;
        };

        template <class Type, int LineNum>
        consteval int NextAutoInjectId()
        {
            if constexpr (LineNum == AUTO_INJECT_MAX_LINES) {
                return AUTO_INJECT_MAX_LINES;
            } else if constexpr (AutoInjectCallable<Type, LineNum + 1>) {
                return LineNum + 1;
            } else {
                return NextAutoInjectId<Type, LineNum + 1>();
            }
        }
        template<class Type>
        concept IsAutoInjectable = (NextAutoInjectId<Type, 1>() != AUTO_INJECT_MAX_LINES);

        template <class Type, int LineNum = NextAutoInjectId<Type, 1>()>
        struct AutoInjecter {};

        template <IsAutoInjectable Type, int LineNum>
        struct AutoInjecter<Type, LineNum>
        {
            void operator()([[maybe_unused]] Type& ret, Container* c)
            {
                if constexpr (AutoInjectCallable<Type, LineNum>) {
                    ret | AutoInjectLine<LineNum>{c};
                }
                [[maybe_unused]] constexpr int nextId = NextAutoInjectId<Type, LineNum>();
                if constexpr (nextId != AUTO_INJECT_MAX_LINES) {
                    AutoInjecter<Type, nextId>{}(ret, c);
                }
            }
        };
    }

    template<detail::IsAutoInjectable Type>
    struct InjectTraits<Type>
    {
        void onInject(Type* value, Container* c)
        {
            detail::AutoInjecter<Type>{}(*value, c);
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
