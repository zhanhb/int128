#pragma once

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <type_traits>

#ifndef __BYTE_ORDER__
#error __BYTE_ORDER__ not defined
#endif

namespace int128_impl {
    template<class, class>
    class int128_base;

    typedef int128_base<int64_t, uint64_t> int128;
    typedef int128_base<uint64_t, uint64_t> uint128;

    template<class _Tp>
    struct signed_helper {
        static constexpr int type() {
            return std::is_integral<_Tp>::value ? 1 + std::is_signed<_Tp>::value : 0;
        }
    };

    template<class _Tp>
    struct signed_helper<const _Tp> : signed_helper<_Tp> {
    };
    template<class _Tp>
    struct signed_helper<volatile _Tp> : signed_helper<_Tp> {
    };
    template<class _Tp>
    struct signed_helper<const volatile _Tp> : signed_helper<_Tp> {
    };

    template<>
    struct signed_helper<uint128> {
        static constexpr int type() { return 1; }
    };

    template<>
    struct signed_helper<int128> {
        static constexpr int type() { return 2; }
    };

    template<class _Tp, bool = signed_helper<_Tp>::type() == 1>
    struct mask_helper;

    template<class _Tp>
    struct mask_helper<_Tp, true> {
        static constexpr _Tp half_mask() { return (_Tp(1) << (4 * sizeof(_Tp))) - _Tp(1); }
    };

    template<class _Tp, int _sign_mask, std::size_t _min_bits, std::size_t _max_bits, class _Up = void>
    using sized_int_helper = typename std::enable_if<
            (_sign_mask & signed_helper<_Tp>::type()) &&
            _min_bits / 8 <= sizeof(_Tp) && sizeof(_Tp) <= _max_bits / 8, _Up>::type;

    template<class _Tp, class _High, class _Low>
    struct cast_helper;

    template<class _Tp>
    struct cast_helper<_Tp, uint64_t, uint64_t> {
        static constexpr _Tp cast(uint64_t high, uint64_t low) { return std::ldexp(_Tp(high), 64) + _Tp(low); }
    };

    template<class _Tp>
    struct cast_helper<_Tp, int64_t, uint64_t> {
        static constexpr _Tp cast(int64_t high, uint64_t low) {
            return high < 0 ? -std::ldexp(_Tp(-high - (low != 0)), 64) - _Tp(-low)
                            : std::ldexp(_Tp(high), 64) + _Tp(low);
        }
    };

    template<class high_type, class low_type>
    class alignas(sizeof(high_type) * 2) int128_base {
        template<class _Tp, std::size_t _min_bits, std::size_t _max_bits = _min_bits, class _Up = void>
        using sized_uint = sized_int_helper<_Tp, 1, _min_bits, _max_bits, _Up>;
        template<class _Tp, std::size_t _min_bits, std::size_t _max_bits = _min_bits, class _Up = void>
        using sized_int = sized_int_helper<_Tp, 2, _min_bits, _max_bits, _Up>;
        template<class _Tp, std::size_t _min_bits, std::size_t _max_bits = _min_bits, class _Up = void>
        using sized_any_sign_int = sized_int_helper<_Tp, 3, _min_bits, _max_bits, _Up>;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        low_type low{};
        high_type high{};
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        high_type high{};
        low_type low{};
#else
#error endian not support
#endif

        constexpr int128_base(high_type high, low_type low) : high(high), low(low) {}

        struct float_tag {
        };
    public:

        constexpr int128_base() noexcept = default;

        constexpr int128_base(const int128_base &) noexcept = default;

        constexpr int128_base(int128_base &&) noexcept = default;

        int128_base &operator=(const int128_base &) noexcept = default;

        int128_base &operator=(int128_base &&) noexcept = default;

        template<class _Tp>
        constexpr int128_base(_Tp x, float_tag) : high(high_type(std::ldexp(x, -64)) - (x < 0)), low(low_type(x)) {}

        constexpr explicit int128_base(float x) : int128_base(x, float_tag()) {}

        constexpr explicit int128_base(double x) : int128_base(x, float_tag()) {}

        constexpr explicit int128_base(long double x) : int128_base(x, float_tag()) {}

        template<class _Integral, sized_int<_Integral, 0, 64> * = nullptr>
        constexpr int128_base(_Integral x): high(-(x < _Integral(0))), low(x) {} // NOLINT

        template<class _Integral, sized_uint<_Integral, 0, 64> * = nullptr>
        constexpr int128_base(_Integral x): high(0), low(x) {} // NOLINT

        template<class _Integral, sized_int<_Integral, 128> * = nullptr>
        constexpr int128_base(_Integral x) : high(high_type(x >> 64)), low(low_type(x)) {} // NOLINT

        template<class _Integral, sized_uint<_Integral, 128> * = nullptr>
        constexpr explicit int128_base(_Integral x) : high(high_type(x >> 64)), low(low_type(x)) {}

        constexpr explicit operator bool() const { return high || low; }

        template<class _Integral, sized_any_sign_int<_Integral, 0, 64, int> = 0>
        constexpr explicit operator _Integral() const { return _Integral(low); }

        template<class _Integral, typename std::enable_if<!std::is_class<_Integral>::value, sized_any_sign_int<_Integral, 128> >::type * = nullptr>
        constexpr explicit operator _Integral() const { return _Integral(high) << 64 | _Integral(low); }

    private:
        template<class _Tp>
        constexpr _Tp cast_to_float() const { return cast_helper<_Tp, high_type, low_type>::cast(high, low); }

    public:
        constexpr explicit operator float() const { return cast_to_float<float>(); }

        constexpr explicit operator double() const { return cast_to_float<double>(); }

        constexpr explicit operator long double() const { return cast_to_float<long double>(); }

        constexpr int128_base operator+() const { return *this; }

        constexpr int128_base operator-() const { return int128_base(-high - (low != 0), -low); }

        constexpr int128_base operator~() const { return int128_base(~high, ~low); }

        constexpr bool operator!() const { return !high && !low; }

        // avoid self plus on rvalue
        int128_base &operator++() &{
            if (!++low) ++high;
            return *this;
        }

        int128_base &operator--() &{
            if (!low--) --high;
            return *this;
        }

        int128_base operator++(int) &{ // NOLINT
            int128_base tmp = *this;
            ++*this;
            return tmp;
        }

        int128_base operator--(int) &{ // NOLINT
            int128_base tmp = *this;
            --*this;
            return tmp;
        }

        friend constexpr int128_base operator+(int128_base a, int128_base b) {
            // no worry for unsigned type, won't be optimized if overflow
            return int128_base(high_type(a.high + b.high + (a.low + b.low < a.low)), a.low + b.low);
        }

        friend constexpr int128_base operator-(int128_base a, int128_base b) {
            return int128_base(high_type(high_type(a.high - b.high - (a.low < b.low))), a.low - b.low);
        }

        friend constexpr int128_base operator*(int128_base a, int128_base b) {
            return int128_base(high_type(a.low * b.high + b.low * a.high) + (a.low >> 32U) * (b.low >> 32U),
                               (a.low & mask_helper<low_type>::half_mask()) *
                               (b.low & mask_helper<low_type>::half_mask()))
                   + (int128_base((a.low >> 32U) * (b.low & mask_helper<low_type>::half_mask())) << 32U)
                   + (int128_base((b.low >> 32U) * (a.low & mask_helper<low_type>::half_mask())) << 32U);
        }

        template<class _Tp, class _Up>
        friend int128_base<_Tp, _Up> operator/(int128_base<_Tp, _Up>, int128_base<_Tp, _Up>);

        template<class _Tp, class _Up>
        friend int128_base<_Tp, _Up> operator%(int128_base<_Tp, _Up>, int128_base<_Tp, _Up>);

        friend constexpr int128_base operator&(int128_base a, int128_base b) {
            return int128_base(a.high & b.high, a.low & b.low);
        }

        friend constexpr int128_base operator|(int128_base a, int128_base b) {
            return int128_base(a.high | b.high, a.low | b.low);
        }

        friend constexpr int128_base operator^(int128_base a, int128_base b) {
            return int128_base(a.high ^ b.high, a.low ^ b.low);
        }

        friend constexpr bool operator<(int128_base a, int128_base b) {
            return a.high < b.high || (a.high == b.high && a.low < b.low);
        }

        friend constexpr bool operator==(int128_base a, int128_base b) { return a.high == b.high && a.low == b.low; }

        friend constexpr bool operator>(int128_base a, int128_base b) { return b < a; }

        friend constexpr bool operator>=(int128_base a, int128_base b) { return !(a < b); }

        friend constexpr bool operator<=(int128_base a, int128_base b) { return !(b < a); }

        friend constexpr bool operator!=(int128_base a, int128_base b) { return !(a == b); }

        friend constexpr int128_base operator<<(int128_base x, unsigned n) {
            // [64,127], 64 {low << 0, 0}
            return n & 64U ? int128_base(high_type(x.low << (n & 63U)), low_type(0)) :
                   n & 63U ? int128_base(high_type((low_type(x.high) << (n & 63U)) | (x.low >> (64U - (n & 63U)))),
                                         x.low << (n & 63U)) : x;
        }

        friend constexpr int128_base operator>>(int128_base x, unsigned n) {
            return n & 64U ? int128_base(high_type(x.high < high_type(0) ? -1 : 0), low_type(x.high >> (n & 63U))) :
                   n & 63U ? int128_base(x.high >> (n & 63U),
                                         (low_type(x.high) << (64 - (n & 63U)) | (x.low >> (n & 63U)))) : x;
        }

        friend constexpr int128_base operator<<(int128_base x, int128_base y) { return x << (unsigned) y.low; }

        friend constexpr int128_base operator>>(int128_base x, int128_base y) { return x >> (unsigned) y.low; }

        friend std::ostream &operator<<(std::ostream &out, int128);

        friend std::ostream &operator<<(std::ostream &out, uint128);

        int128_base &operator+=(int128_base x) &{ return *this = *this + x; }

        int128_base &operator-=(int128_base x) &{ return *this = *this - x; }

        int128_base &operator*=(int128_base x) &{ return *this = *this * x; }

        int128_base &operator/=(int128_base another) &{ return *this = *this / another; }

        int128_base &operator%=(int128_base another) &{ return *this = *this % another; }

        int128_base &operator<<=(int128_base x) &{ return *this = *this << x; }

        int128_base &operator>>=(int128_base x) &{ return *this = *this >> x; }

        int128_base &operator<<=(unsigned x) &{ return *this = *this << x; }

        int128_base &operator>>=(unsigned x) &{ return *this = *this >> x; }

        int128_base &operator&=(int128_base another) &{ return *this = *this & another; }

        int128_base &operator|=(int128_base another) &{ return *this = *this | another; }

        int128_base &operator^=(int128_base another) &{ return *this = *this ^ another; }

        friend class bit_operator;
    };
}

namespace int128_impl {
    template<char ch>
    struct static_digit_internal {
        constexpr static int value() {
            return '0' <= ch && ch <= '9' ? ch - '0' : 'a' <= ch && ch <= 'z' ? ch - 'a' + 10 :
                                                       'A' <= ch && ch <= 'Z' ? ch - 'A' + 10 : -1;
        }
    };

    template<int base, char ch>
    struct static_digit {
        constexpr static int value() {
            static_assert(0 <= static_digit_internal<ch>::value()
                          && static_digit_internal<ch>::value() < base, "find character not digit");
            return static_digit_internal<ch>::value();
        }
    };

    template<class type, int base, char ...Args>
    struct int128_literal_based {
        static constexpr type value() { return type(0); };

        static constexpr type base_pow() { return type(1); };
    };

    template<class type, int base, char ch, char ...rest>
    struct int128_literal_based<type, base, ch, rest...> {
        using target = int128_literal_based<type, base, rest...>;

        static constexpr type value() {
            return target::value() + target::base_pow() * static_digit<base, ch>::value();
        };

        static constexpr type base_pow() {
            return target::base_pow() * base;
        };
    };

    template<class type, char ...Args>
    struct int128_literal : int128_literal_based<type, 10, Args...> {
    };
    template<class type, char ...Args>
    struct int128_literal<type, '0', Args...> : int128_literal_based<type, 8, Args...> {
    };
    template<class type, char ...Args>
    struct int128_literal<type, '0', 'x', Args...> : int128_literal_based<type, 16, Args...> {
    };
    template<class type, char ...Args>
    struct int128_literal<type, '0', 'X', Args...> : int128_literal_based<type, 16, Args...> {
    };
    template<class type, char ...Args>
    struct int128_literal<type, '0', 'b', Args...> : int128_literal_based<type, 2, Args...> {
    };
    template<class type, char ...Args>
    struct int128_literal<type, '0', 'B', Args...> : int128_literal_based<type, 2, Args...> {
    };

    template<char ... Args>
    inline constexpr uint128 operator "" _u128() { return int128_literal<uint128, Args...>::value(); }

    template<char ...Args>
    inline constexpr int128 operator "" _l128() { return int128_literal<int128, Args...>::value(); }

    template<char ... Args>
    inline constexpr uint128 operator "" _U128() { return int128_literal<uint128, Args...>::value(); }

    template<char ...Args>
    inline constexpr int128 operator "" _L128() { return int128_literal<int128, Args...>::value(); }

    struct bit_operator {

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

        static constexpr int clz64(uint64_t x) {
            static_assert(sizeof(uint64_t) == sizeof(long) || sizeof(uint64_t) == sizeof(long long),
                          "neither of long or long long fit 64 bit integer");
            return sizeof(uint64_t) == sizeof(long) ? __builtin_clzl(x) : __builtin_clzll(x);
        }

#pragma clang diagnostic pop

        static constexpr int clz128(uint128 v) { return v.high ? clz64(v.high) : 4 * sizeof(v) + clz64(v.low); }
    };

    inline uint128 &div_mod_impl(uint128 &dividend, uint128 divisor, uint128 &quot) {
        if (!dividend) return dividend;
        auto zend = bit_operator::clz128(dividend), zsor = bit_operator::clz128(divisor);
        if (zend > zsor) return dividend;
        for (zsor -= zend, divisor <<= zsor;; divisor >>= 1, quot <<= 1) {
            if (dividend >= divisor) {
                dividend -= divisor;
                quot |= 1;
            }
            if (!zsor--) return dividend;
        }
    }

    inline uint128 operator/(uint128 n, uint128 d) {
        if (!d) return !!n / !!d; // raise signal SIGFPE
        uint128 quot = 0;
        div_mod_impl(n, d, quot);
        return quot;
    };

    inline uint128 operator%(uint128 n, uint128 d) {
        if (!d) return !!n % !!d; // raise signal SIGFPE
        uint128 quot = 0;
        return div_mod_impl(n, d, quot);
    };

    inline int128 operator%(int128 n, int128 d) {
        bool negative = n < int128(0);
        auto tmp = uint128(negative ? -n : n) % uint128(d < int128(0) ? -d : d);
        return int128(negative ? -tmp : tmp);
    }

    inline uint128 operator/(int128 n, int128 d) {
        bool nnegative = n < int128(0), dnegative = d < int128(0);
        bool rnegative = nnegative != dnegative;
        auto result = uint128(nnegative ? -n : n) / uint128(dnegative ? -d : d);
        return int128(rnegative ? -result : result);
    };

    inline std::ostream &print_value(std::ostream &out, bool negative, uint64_t high, uint64_t low) {
        // high is not zero
        const std::size_t buf_size = 50;

        std::ios::fmtflags flags = out.flags(), baseFlag = flags & std::ios::basefield;
        std::ios::fmtflags adjust_field = flags & std::ios::adjustfield;
        auto show_base = bool(flags & std::ios::showbase); // work not oct
        auto show_pos = bool(flags & std::ios::showpos); // work only oct
        auto width = out.width(0);

        char buf[buf_size] = {};
        const char *prefix = nullptr;
        int offset = 0;

        switch (baseFlag) {
            case std::ios::hex: {
                auto is_uppercase = bool(flags & std::ios::uppercase);
                if (show_base) prefix = is_uppercase ? "0X" : "0x";
                offset = snprintf(buf, buf_size,
                                  is_uppercase ? "%" PRIX64 "%016" PRIX64 : "%" PRIx64 "%016" PRIx64, high, low);
                break;
            }
            case std::ios::oct: {
                const unsigned sz = sizeof(uint64_t) * 8;
                const unsigned a = sz % 3;
                const unsigned b = sz - a;
                const unsigned c = sz - 2 * a;
                const uint64_t msk1 = (uint64_t(1) << b) - 1;
                const uint64_t msk2 = (uint64_t(1) << c) - 1;
                if (show_base) buf[offset++] = '0';
                if (high & ~msk2) {
                    offset += snprintf(buf + offset, buf_size - offset, "%" PRIo64 "%0*" PRIo64 "%0*" PRIo64,
                                       high >> c, b / 3, ((high & msk2) << a) | (low >> b), b / 3, low & msk1);
                } else {
                    offset += snprintf(buf + offset, buf_size - offset, "%" PRIo64 "%0*" PRIo64,
                                       (high << a) | (low >> b), b / 3, low & msk1);
                }
                break;
            }
            default: {
                if (negative) {
                    prefix = "-";
                    high = ~high + !low;
                    low = ~low + 1;
                } else if (show_pos) {
                    prefix = "+";
                }
                uint128 llll = uint128(high) << 64 | uint128(low);
                const uint128 div(UINT64_C(10000000000000000000));
                uint128 mid = 0, hhhh = 0;
                div_mod_impl(llll, div, mid);
                if (mid) {
                    div_mod_impl(mid, div, hhhh);
                    if (hhhh) {
                        offset = snprintf(buf, buf_size, "%" PRIu64 "%019" PRIu64 "%019" PRIu64, (uint64_t) hhhh,
                                          (uint64_t) mid, (uint64_t) llll);
                    } else {
                        offset = snprintf(buf, buf_size, "%" PRIu64 "%019" PRIu64, (uint64_t) mid, (uint64_t) llll);
                    }
                } else {
                    // when high is -1 and low is not 0
                    offset = snprintf(buf, buf_size, "%" PRIu64, (uint64_t) llll);
                }
                break;
            }
        }
        if (!prefix) {
            // revert width
            out.width(width);
            return out << buf;
        }
        auto prefixLen = strlen(prefix);
        auto total = prefixLen + offset;
        if (width > total) {
            auto extra = width - total;
            switch (adjust_field) {
                case std::ios::left:
                case std::ios::internal: {
                    // width already set to 0
                    out << prefix;
                    out.width(extra + offset);
                    return out << buf;
                }
                default: {
                    out.width(extra + prefixLen);
                    return out << prefix << buf;
                }
            }
        } else {
            // width already set to 0
            return out << prefix << buf;
        }
    }

    inline std::ostream &operator<<(std::ostream &out, uint128 iii) {
        if (!iii.high) return out << iii.low;
        print_value(out, false, iii.high, iii.low);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, int128 iii) {
        if (!iii.high) return out << iii.low;
        print_value(out, iii.high < INT64_C(0), uint64_t(iii.high), iii.low);
        return out;
    }
}

#ifdef INT128_SPECIALIZATION
namespace std {
#pragma push_macro("MAKE_TYPE")
#define MAKE_TYPE(outter, inner, parent) \
template<> struct outter<int128_impl::inner> : std::parent {}; \
template<> struct outter<const int128_impl::inner> : std::parent {}; \
template<> struct outter<volatile int128_impl::inner> : std::parent {}; \
template<> struct outter<const volatile int128_impl::inner> : std::parent {};
    MAKE_TYPE(is_integral, uint128, true_type)
    MAKE_TYPE(is_integral, int128, true_type)
    MAKE_TYPE(is_signed, uint128, false_type)
    MAKE_TYPE(is_signed, int128, true_type)
#undef MAKE_TYPE
#define MAKE_TYPE(outter, inner, target) \
template<> struct outter<int128_impl::inner> { typedef int128_impl::target type; }; \
template<> struct outter<const int128_impl::inner> { typedef const int128_impl::target type; }; \
template<> struct outter<volatile int128_impl::inner> { typedef volatile int128_impl::target type; }; \
template<> struct outter<const volatile int128_impl::inner> { typedef const volatile int128_impl::target type; };
    MAKE_TYPE(make_signed, uint128, int128)
    MAKE_TYPE(make_unsigned, int128, uint128)
#pragma pop_macro("MAKE_TYPE")
}
#endif

#ifndef INT128_NO_EXPORT
#define INT128_C(val) val##_L128
#define UINT128_C(val) val##_U128
// add space between ‘""’ and suffix identifier, or may compile failed
using int128_impl::operator "" _L128;
using int128_impl::operator "" _l128;
using int128_impl::operator "" _U128;
using int128_impl::operator "" _u128;
using int128_impl::uint128;
using int128_impl::int128;
#endif
