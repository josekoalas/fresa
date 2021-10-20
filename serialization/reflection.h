//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

//thanks to Jameson Thatcher (bluescreenofdoom)[http://bluescreenofdoom.com/post/code/Reflection/],
//          veselink1 (refl-cpp)[https://github.com/veselink1/refl-cpp],
//          chochlik (static reflection)[https://isocpp.org/files/papers/n3996.pdf] and
//          KonanM (tser)[https://github.com/KonanM/tser]
//for the help with his part

#pragma once

#include <array>
#include <tuple>
#include <ostream>

#define Serialize(Type, ...) \
\
inline decltype(auto) members() const { return std::tie(__VA_ARGS__); } \
inline decltype(auto) members() { return std::tie(__VA_ARGS__); } \
\
static constexpr std::array<char, ::Verse::Reflection::Impl::str_size(#__VA_ARGS__)> member_name_data = [](){ \
    std::array<char, ::Verse::Reflection::Impl::str_size(#__VA_ARGS__)> chars{'\0'}; size_t _idx = 0; \
    constexpr auto* ini(#__VA_ARGS__); \
    for (char const* _c = ini; *_c; ++_c, ++_idx) \
        if(*_c != ',' && *_c != ' ') \
            chars[_idx] = *_c; \
    return chars;}(); \
\
static constexpr const char* type_name = #Type; \
\
static constexpr std::array<const char*, ::Verse::Reflection::Impl::n_args(#__VA_ARGS__)> member_names = [](){ \
    std::array<const char*, ::Verse::Reflection::Impl::n_args(#__VA_ARGS__)> out{ }; \
    for(size_t i = 0, n_args = 0; n_args < ::Verse::Reflection::Impl::n_args(#__VA_ARGS__) ; ++i) { \
        while(Type::member_name_data[i] == '\0') i++; \
        out[n_args++] = &Type::member_name_data[i]; \
        while(Type::member_name_data[++i] != '\0'); } \
    return out;}(); \
\
template<typename OT, std::enable_if_t<std::is_same_v<OT,Type> && !::Verse::Reflection::is_detected<::Verse::Reflection::t_equal, OT>, int> = 0> \
friend bool operator==(const Type& lhs, const OT& rhs) { return lhs.members() == rhs.members(); } \
template<typename OT, std::enable_if_t<std::is_same_v<OT,Type> && !::Verse::Reflection::is_detected<::Verse::Reflection::t_nequal, OT>, int> = 0> \
friend bool operator!=(const Type& lhs, const OT& rhs) { return !(lhs == rhs); } \
template<typename OT, std::enable_if_t<std::is_same_v<OT,Type> && !::Verse::Reflection::is_detected<::Verse::Reflection::t_smaller, OT>, int> = 0> \
friend bool operator< (const OT& lhs, const OT& rhs) { return ::Verse::Reflection::less(lhs, rhs); } \
template<typename OT, std::enable_if_t<std::is_same_v<OT,Type> && !::Verse::Reflection::is_detected<::Verse::Reflection::t_print, OT>, int> = 0> \
friend std::ostream& operator<<(std::ostream& os, const OT& t) { ::Verse::Reflection::print(os, t); return os; }

namespace Verse
{
    namespace Reflection
    {
        //C++20 is_detected implementation
        namespace Impl
        {
            struct ns {
                ~ns() = delete;
                ns(ns const&) = delete;
            };
        
            template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
            struct detector {
                using value_t = std::false_type;
                using type = Default;
            };
        
            template <class Default, template<class...> class Op, class... Args>
            struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
                using value_t = std::true_type;
                using type = Op<Args...>;
            };
        
            template<class T>
            struct is_array : std::is_array<T> {};
        
            template<template<typename, size_t> class TArray, typename T, size_t N>
            struct is_array<TArray<T, N>> : std::true_type {};
        
            constexpr size_t n_args(char const* c, size_t nargs = 1) {
                for (; *c; ++c) if (*c == ',') ++nargs;
                return nargs;
            }
        
            constexpr size_t str_size(char const* c, size_t strSize = 1) {
                for (; *c; ++c) ++strSize; return strSize;
            }
        }
    
        template <template<class...> class Op, class... Args>
        constexpr bool is_detected = Impl::detector<Impl::ns, void, Op, Args...>::value_t::value;
    
        //Properties
        template<class T> using t_begin = decltype(*std::begin(std::declval<T>()));
        template<class T> constexpr bool is_container = is_detected<t_begin, T>;
    
        template<class T> using t_members = decltype(std::declval<T>().members());
        template<class T> constexpr bool is_reflectable = is_detected<t_members, T>;
        
        template<class T> using t_tuple = std::tuple_element_t<0, T>;
        template<class T> constexpr bool is_tuple = is_detected<t_tuple, T>;
    
        template<class T> using t_print = decltype(std::declval<std::ostream>() << std::declval<T>());
    
        template<class T> using t_optional = decltype(std::declval<T>().has_value());
        template<class T> using t_element = typename T::element_type;
        template<class T> using t_mapped = typename T::mapped_type;
        template<class T> constexpr bool is_pointer_like = std::is_pointer_v<T> || is_detected<t_element, T> || is_detected<t_optional, T>;
    
        template<class T> using t_smaller = decltype(std::declval<T>() < std::declval<T>());
        template<class T> using t_equal = decltype(std::declval<T>() == std::declval<T>());
        template<class T> using t_nequal = decltype(std::declval<T>() != std::declval<T>());
    
        /*template<class T> using t_custom_save = decltype(std::declval<T>().save(std::declval<BinaryArchive&>()));
         template<class T> using t_free_save = decltype(std::declval<const T&>() << std::declval<BinaryArchive&>());*/
    
        //Tuple less operator
        template<typename T>
        constexpr inline bool less(const T& lhs, const T& rhs);
    
        template< class T, std::size_t... I>
        constexpr inline bool less(const T& lhs, const T& rhs, std::index_sequence<I...>) {
            bool is_smaller = false;
            (void)((less(std::get<I>(lhs) , std::get<I>(rhs)) ? (static_cast<void>(is_smaller = true), false) :
                                                                (less(std::get<I>(rhs), std::get<I>(lhs)) ? false : true)) &&...);
            return is_smaller;
        }
    
        template<typename T>
        constexpr inline bool less(const T& lhs, const T& rhs) {
            if constexpr (is_reflectable<T>)
                return less(lhs.members(), rhs.members());
            else if constexpr (is_tuple<T>)
                return less(lhs, rhs, std::make_index_sequence<std::tuple_size_v<T>>());
            else if constexpr (is_container<T> && !is_detected<t_smaller, T>) {
                if (lhs.size() != rhs.size())
                    return lhs.size() < rhs.size();
                return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
            }
            else if constexpr (std::is_enum_v<T>)
                return static_cast<std::underlying_type_t<T>>(lhs) < static_cast<std::underlying_type_t<T>>(rhs);
            else
                return lhs < rhs;
        }
    
        //Recursively print JSON (TODO: CHANGE)
        template<typename T>
        constexpr inline decltype(auto) print(std::ostream& os, T&& val) {
            using V = std::decay_t<T>;
            if constexpr (std::is_constructible_v<std::string, T> || std::is_same_v<V, char>)
                os << "\"" << val << "\"";
            else if constexpr (is_container<V>) {
                size_t i = 0;
                os << "[";
                for (auto& elem : val)
                    os << (i++ == 0 ? "" : ",") << ::Verse::Reflection::print(os, elem);
                os << "]";
            }
            else if constexpr (is_reflectable<V> && !is_detected<t_print, V>) {
                auto p_mem = [&](auto& ... member) {
                    size_t i = 0;
                    (((os << (i != 0 ? ", " : "") << '\"'), os << V::member_names[i++] << "\" : " << ::Verse::Reflection::print(os, member)), ...);
                };
                os << "{ \"" << V::type_name << "\": {"; std::apply(p_mem, val.members()); os << "}}";
            }
            else if constexpr (std::is_enum_v<V> && !is_detected<t_print, V>) {
                os << ::Verse::Reflection::print(os, static_cast<std::underlying_type_t<V>>(val));
            }
            else if constexpr (is_tuple<V> && !is_detected<t_print, V>) {
                std::apply([&](auto& ... t) {
                    int i = 0; os << "{"; (((i++ != 0 ? os << ", " : os), ::Verse::Reflection::print(os, t)), ...); os << "}";
                }, val);
            }
            else if constexpr (is_pointer_like<V>) {
                os << (val ? (os << (::Verse::Reflection::print(os, *val)), "") : "null");
            }
            else
                os << val;
            return "";
        }
    
    }
}