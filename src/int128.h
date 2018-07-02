#pragma once

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
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
    struct half_mask {
        static constexpr _Tp value() { return (_Tp(1) << (4 * sizeof(_Tp))) - _Tp(1); }
    };

    template<class _Tp, class _High, class _Low>
    struct float_cast_helper;

    template<class _Tp>
    struct float_cast_helper<_Tp, uint64_t, uint64_t> {
        static constexpr _Tp cast(uint64_t high, uint64_t low) { return std::ldexp(_Tp(high), 64) + _Tp(low); }
    };

    template<class _Tp>
    struct float_cast_helper<_Tp, int64_t, uint64_t> {
        static constexpr _Tp cast(int64_t high, uint64_t low) {
            return high < 0 ? -std::ldexp(_Tp(-high - (low != 0)), 64) - _Tp(-low)
                            : std::ldexp(_Tp(high), 64) + _Tp(low);
        }
    };

    template<class _Hi, class _Low>
    class alignas(sizeof(_Hi) * 2) int128_base final {
        static_assert(sizeof(_Hi) == sizeof(_Low), "low type, high type should have same size");

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        _Low low{};
        _Hi high{};
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        _Hi high{};
        _Low low{};
#else
#error endian not support
#endif

        constexpr int128_base(_Hi high, _Low low) : high(high), low(low) {}

        struct integral {};
        struct signed_integral : integral {};
        struct unsigned_integral : integral {};
        struct float_tag {
        };
        template<size_t>
        struct size_constant {};

        template<class _Tp>
        constexpr int128_base(_Tp value_, signed_integral, size_constant<8>):
                high(value_ < 0 ? -1 : 0), low(value_) {}

        template<class _Tp>
        constexpr int128_base(_Tp value_, unsigned_integral, size_constant<8>) :
                high(0), low(value_) {}

        template<class _Tp>
        constexpr int128_base(_Tp value_, integral, size_constant<16>) :
                high(value_ >> 64U), low(value_) {}

    public:

        constexpr int128_base() noexcept = default;

        constexpr int128_base(const int128_base &) noexcept = default;

        constexpr int128_base(int128_base &&) noexcept = default;

        template<class _Tp>
        constexpr explicit int128_base(int128_base<_Tp, _Low> v) : high(v.high), low(v.low) {}

        int128_base &operator=(const int128_base &) noexcept = default;

        int128_base &operator=(int128_base &&) noexcept = default;

        template<class _Tp>
        constexpr int128_base(_Tp x, float_tag) : high(_Hi(std::ldexp(x, -64)) - (x < 0)), low(_Low(x)) {}

        constexpr explicit int128_base(float x) : int128_base(x, float_tag()) {}

        constexpr explicit int128_base(double x) : int128_base(x, float_tag()) {}

        constexpr explicit int128_base(long double x) : int128_base(x, float_tag()) {}

        constexpr int128_base(long long value_) : // NOLINT
                int128_base(value_, signed_integral(), size_constant<sizeof(value_)>()) {}

        constexpr int128_base(long value_) : int128_base(static_cast<long long>(value_)) {} // NOLINT

        constexpr int128_base(int value_) : int128_base(long(value_)) {} // NOLINT

        constexpr int128_base(unsigned long long value_) : // NOLINT
                int128_base(value_, unsigned_integral(), size_constant<sizeof(value_)>()) {}

        constexpr int128_base(unsigned long value_) : int128_base(static_cast<unsigned long long>(value_)) {} // NOLINT

        constexpr int128_base(unsigned value_) : int128_base(static_cast<unsigned long>(value_)) {} // NOLINT

#if __SIZEOF_INT128__ == 16

        constexpr int128_base(__int128 value_) :
                int128_base(value_, signed_integral(), size_constant<sizeof(value_)>()) {} // NOLINT

        constexpr explicit int128_base(unsigned __int128 value_) :
                int128_base(value_, unsigned_integral(), size_constant<sizeof(value_)>()) {}
#endif

        constexpr explicit operator bool() const { return high || low; }

        constexpr explicit operator char() const { return char(low); }

        constexpr explicit operator signed char() const { return static_cast<signed char>(low); }

        constexpr explicit operator unsigned char() const { return static_cast<unsigned char>(low); }

        constexpr explicit operator short() const { return short(low); }

        constexpr explicit operator unsigned short() const { return static_cast<unsigned short>(low); }

        constexpr explicit operator int() const { return int(low); }

        constexpr explicit operator unsigned() const { return unsigned(low); }

        constexpr explicit operator long() const { return long(low); }

        constexpr explicit operator unsigned long() const { return static_cast<unsigned long>(low); }

        constexpr explicit operator long long() const { return static_cast<long long>(low); }

        constexpr explicit operator unsigned long long() const { return static_cast<unsigned long long>(low); }

        constexpr explicit operator wchar_t() const { return wchar_t(low); }

        constexpr explicit operator char16_t() const { return char16_t(low); }

        constexpr explicit operator char32_t() const { return char32_t(low); }

#if __SIZEOF_INT128__ == 16

        constexpr explicit operator __int128() const { return __int128(high) << 64 | __int128(low); }

        explicit operator unsigned __int128() const { // NOLINT
            return static_cast<unsigned __int128>(high) << 64 | static_cast<unsigned __int128>(low);
        }
#endif

    private:
        template<class _Tp>
        constexpr _Tp cast_to_float() const { return float_cast_helper<_Tp, _Hi, _Low>::cast(high, low); }

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
            return int128_base(_Hi(a.high + b.high + (a.low + b.low < a.low)), a.low + b.low);
        }

        friend constexpr int128_base operator-(int128_base a, int128_base b) {
            return int128_base(_Hi(_Hi(a.high - b.high - (a.low < b.low))), a.low - b.low);
        }

        friend constexpr int128_base operator*(int128_base a, int128_base b) {
            return int128_base(_Hi(a.low * b.high + b.low * a.high) + (a.low >> 32U) * (b.low >> 32U),
                               (a.low & half_mask<_Low>::value()) *
                               (b.low & half_mask<_Low>::value()))
                   + (int128_base((a.low >> 32U) * (b.low & half_mask<_Low>::value())) << 32U)
                   + (int128_base((b.low >> 32U) * (a.low & half_mask<_Low>::value())) << 32U);
        }

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
            return n & 64U ? int128_base(_Hi(x.low << (n & 63U)), _Low(0)) :
                   n & 63U ? int128_base(_Hi((_Low(x.high) << (n & 63U)) | (x.low >> (64U - (n & 63U)))),
                                         x.low << (n & 63U)) : x;
        }

        friend constexpr int128_base operator<<(int128_base x, int128_base y) { return x << (unsigned) y.low; }

        friend constexpr int128_base operator>>(int128_base x, int128_base y) { return x >> (unsigned) y.low; }

        int128_base &operator+=(int128_base x) &{ return *this = *this + x; }

        int128_base &operator-=(int128_base x) &{ return *this = *this - x; }

        int128_base &operator*=(int128_base x) &{ return *this = *this * x; }

        int128_base &operator/=(int128_base another) &{ return *this = *this / another; }

        int128_base &operator%=(int128_base another) &{ return *this = *this % another; }

        int128_base &operator<<=(int128_base x) &{ return *this = *this << x; }

        friend constexpr uint128 operator>>(uint128 x, unsigned n);

        friend constexpr int128 operator>>(int128 x, unsigned n);

        int128_base &operator>>=(int128_base x) &{ return *this = *this >> x; }

        int128_base &operator<<=(unsigned x) &{ return *this = *this << x; }

        int128_base &operator>>=(unsigned x) &{ return *this = *this >> x; }

        int128_base &operator&=(int128_base another) &{ return *this = *this & another; }

        int128_base &operator|=(int128_base another) &{ return *this = *this | another; }

        int128_base &operator^=(int128_base another) &{ return *this = *this ^ another; }

        template<class, class>
        friend
        class int128_base;

        template<class>
        friend
        class clz_helper;
    };

    template<char _Ch, int _Rad>
    struct static_digit {
        static const int value_ =
                '0' <= _Ch && _Ch <= '9' ? _Ch - '0' :
                'a' <= _Ch && _Ch <= 'z' ? _Ch - 'a' + 10 :
                'A' <= _Ch && _Ch <= 'Z' ? _Ch - 'A' + 10 : -1;

        static_assert(0 <= value_ && value_ < _Rad, "character not a digit");

        static constexpr int value() { return value_; }
    };

    template<class, int, char ...>
    struct int128_literal_radix;

    template<class _Tp, int _Rad, char _Ch>
    struct int128_literal_radix<_Tp, _Rad, _Ch> {
        static constexpr _Tp value() { return _Tp(static_digit<_Ch, _Rad>::value()); };

        static constexpr _Tp append(_Tp v) { return v * _Tp(_Rad) + value(); };
    };

    template<class _Tp, int _Rad, char _Ch, char ..._Args>
    struct int128_literal_radix<_Tp, _Rad, _Ch, _Args...> {
        using _Cur = int128_literal_radix<_Tp, _Rad, _Ch>;
        using _Tgt = int128_literal_radix<_Tp, _Rad, _Args...>;

        static constexpr _Tp value() { return _Tgt::append(_Cur::value()); };

        static constexpr _Tp append(_Tp v) { return _Tgt::append(v * _Tp(_Rad) + _Cur::value()); };
    };

    template<class _Tp, char ..._Args>
    struct int128_literal : int128_literal_radix<_Tp, 10, _Args...> {
    };
    template<class _Tp>
    struct int128_literal<_Tp, '0'> : int128_literal_radix<_Tp, 10, '0'> {
    };
    template<class _Tp, char ..._Args>
    struct int128_literal<_Tp, '0', _Args...> : int128_literal_radix<_Tp, 8, _Args...> {
    };
    template<class _Tp, char ..._Args>
    struct int128_literal<_Tp, '0', 'x', _Args...> : int128_literal_radix<_Tp, 16, _Args...> {
    };
    template<class _Tp, char ..._Args>
    struct int128_literal<_Tp, '0', 'X', _Args...> : int128_literal_radix<_Tp, 16, _Args...> {
    };
    template<class _Tp, char ..._Args>
    struct int128_literal<_Tp, '0', 'b', _Args...> : int128_literal_radix<_Tp, 2, _Args...> {
    };
    template<class _Tp, char ..._Args>
    struct int128_literal<_Tp, '0', 'B', _Args...> : int128_literal_radix<_Tp, 2, _Args...> {
    };

    template<char ..._Args>
    inline constexpr uint128 operator "" _u128() { return int128_literal<uint128, _Args...>::value(); }

    template<char ..._Args>
    inline constexpr int128 operator "" _l128() { return int128_literal<int128, _Args...>::value(); }

    template<char ..._Args>
    inline constexpr uint128 operator "" _U128() { return int128_literal<uint128, _Args...>::value(); }

    template<char ..._Args>
    inline constexpr int128 operator "" _L128() { return int128_literal<int128, _Args...>::value(); }

    template<class>
    class clz_helper;

    template<>
    struct clz_helper<unsigned long> {
        static constexpr int clz(unsigned long x) { return __builtin_clzl(x); }
    };

    template<>
    struct clz_helper<unsigned long long> {
        static constexpr int clz(unsigned long long x) { return __builtin_clzll(x); }
    };

    template<class _High, class _Low>
    struct clz_helper<int128_base<_High, _Low> > {
        static constexpr int clz(int128_base<_High, _Low> v) {
            return v.high ? clz_helper<_Low>::clz(v.high) : 4 * sizeof(v) + clz_helper<_Low>::clz(v.low);
        }
    };

    constexpr uint128 operator>>(uint128 x, unsigned n) {
        return n & 64U ? uint128(0, x.high >> (n & 63U)) :
               n & 63U ? uint128(x.high >> (n & 63U),
                                 (x.high << (64 - (n & 63U)) | (x.low >> (n & 63U)))) : x;
    }

    constexpr int128 operator>>(int128 x, unsigned n) {
        return n & 64U ? int128(x.high < 0 ? -1 : 0, uint64_t(x.high >> (n & 63U))) :
               n & 63U ? int128(x.high >> (n & 63U),
                                (uint64_t(x.high) << (64 - (n & 63U)) | (x.low >> (n & 63U)))) : x;
    }

    inline uint128 &div_mod_impl(uint128 &dividend, uint128 divisor, uint128 &quot) {
        // assert(divisor != uint128(0));
        if (!dividend) return dividend;
        auto zend = clz_helper<uint128>::clz(dividend), zsor = clz_helper<uint128>::clz(divisor);
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

    inline int128 operator/(int128 n, int128 d) {
        bool nnegative = n < int128(0), dnegative = d < int128(0);
        bool rnegative = nnegative != dnegative;
        auto result = uint128(nnegative ? -n : n) / uint128(dnegative ? -d : d);
        return int128(rnegative ? -result : result);
    };

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &
    print_value(std::basic_ostream<_CharT, _Traits> &out_, bool signed_integral_, uint128 value_) {
        const std::size_t buf_size_ = 45;

        auto flags_ = out_.flags(), base_flag_ = flags_ & std::ios::basefield;
        std::ios::fmtflags adjust_field_ = flags_ & std::ios::adjustfield;
        auto show_base_ = bool(flags_ & std::ios::showbase); // work not oct
        auto show_pos_ = bool(flags_ & std::ios::showpos); // work only oct
        auto upper_case_ = bool(flags_ & std::ios::uppercase); // work only hex
        auto ns_ = out_.width(0);
        auto fl_ = out_.fill();

        char buf_[buf_size_];
        const char *prefix_ = nullptr;
        int offset_ = 0;

        switch (base_flag_) {
            case std::ios::hex: {
                if (show_base_ && value_) prefix_ = upper_case_ ? "0X" : "0x";
                if (value_ >> 64) {
                    offset_ = snprintf(buf_, buf_size_,
                                       upper_case_ ? "%" PRIX64 "%016" PRIX64 : "%" PRIx64 "%016" PRIx64,
                                       (uint64_t) (value_ >> 64), (uint64_t) value_);
                } else {
                    offset_ = snprintf(buf_, buf_size_,
                                       upper_case_ ? "%" PRIX64 : "%" PRIx64, (uint64_t) value_);
                }
                break;
            }
            case std::ios::oct: {
                const uint64_t mask_ = (UINT64_C(1) << 63U) - 1;
                if (show_base_ && value_) buf_[offset_++] = '0';
                auto x_ = (uint64_t) (value_ >> 126U);
                auto y_ = (uint64_t) ((value_ >> 63U) & mask_);
                auto z_ = (uint64_t) (value_ & mask_);
                if (x_) {
                    offset_ += snprintf(buf_ + offset_, buf_size_ - offset_, "%" PRIo64 "%021" PRIo64 "%021" PRIo64, x_,
                                        y_, z_);
                } else if (y_) {
                    offset_ += snprintf(buf_ + offset_, buf_size_ - offset_, "%" PRIo64 "%021" PRIo64, y_, z_);
                } else {
                    offset_ += snprintf(buf_ + offset_, buf_size_ - offset_, "%" PRIo64, z_);
                }
                break;
            }
            default: {
                if (signed_integral_) {
                    if (value_ >> 127) { // negative
                        prefix_ = "-";
                        value_ = -value_;
                    } else if (show_pos_) {
                        prefix_ = "+";
                    }
                }
                const uint128 div_(UINT64_C(10000000000000000000));
                uint128 mid_ = 0, high_ = 0;
                div_mod_impl(value_, div_, mid_);
                div_mod_impl(mid_, div_, high_);
                if (high_) {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64 "%019" PRIu64 "%019" PRIu64,
                                       (uint64_t) high_, (uint64_t) mid_, (uint64_t) value_);
                } else if (mid_) {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64 "%019" PRIu64,
                                       (uint64_t) mid_, (uint64_t) value_);
                } else {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64, (uint64_t) value_);
                }
                break;
            }
        }

        _CharT o_[2 * buf_size_ - 3];
        _CharT *os_;
        _CharT *op_;  // prefix here
        _CharT *oe_ = o_ + (sizeof(o_) / sizeof(o_[0]));  // end of output

        auto loc_ = out_.getloc();
        auto &ct_ = std::use_facet<std::ctype<_CharT> >(loc_);
        auto &npt_ = std::use_facet<std::numpunct<_CharT> >(loc_);
        std::string grouping_ = npt_.grouping();

        if (grouping_.empty()) {
            op_ = oe_ - offset_;
            ct_.widen(buf_, buf_ + offset_, op_);
        } else {
            op_ = oe_;
            auto thousands_sep_ = npt_.thousands_sep();
            // no worry group is not empty
            auto sz_ = grouping_.size() - 1;
            decltype(sz_) dg_ = 0;
            auto cnt_ = static_cast<unsigned char>(grouping_[dg_]);
            unsigned char dc_ = 0;
            for (char *p_ = buf_ + offset_; p_ != buf_; ++dc_) {
                if (cnt_ > 0 && dc_ == cnt_) {
                    *--op_ = thousands_sep_;
                    dc_ = 0;
                    if (dg_ < sz_) cnt_ = static_cast<unsigned char>(grouping_[++dg_]);
                }
                *--op_ = ct_.widen(*--p_);
            }
        }

        if (prefix_) {
            auto prefix_len_ = strlen(prefix_);
            os_ = op_ - prefix_len_;
            ct_.widen(prefix_, prefix_ + prefix_len_, os_);
        } else {
            os_ = op_;
        }

        auto sz_ = static_cast<std::streamsize>(oe_ - os_);
        // assert(sz_ <= (sizeof(o_) / sizeof(o_[0])));

        if (ns_ > sz_) {
            ns_ -= sz_;
            std::basic_string<_CharT, _Traits> sp_(ns_, fl_);
            switch (adjust_field_) {
                case std::ios::left:
                    return out_.write(os_, sz_).write(sp_.data(), ns_);
                case std::ios::internal:
                    return out_.write(os_, static_cast<std::streamsize>(op_ - os_))
                            .write(sp_.data(), ns_)
                            .write(op_, static_cast<std::streamsize>(oe_ - op_));
                default: {
                    return out_.write(sp_.data(), ns_).write(os_, sz_);
                }
            }
        }
        return out_.write(os_, sz_);
    }

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &out, uint128 _Val) {
        return print_value(out, false, _Val);
    }

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &out, int128 _Val) {
        return print_value(out, true, uint128(_Val));
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
#endif /* INT128_SPECIALIZATION */

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
#endif /* INT128_NO_EXPORT */
