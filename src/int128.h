#pragma once

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include <type_traits>

#ifndef __BYTE_ORDER__
#error __BYTE_ORDER__ not defined
#endif

namespace large_int {
    template<class, class>
    class int128_base;

    typedef int128_base<int64_t, uint64_t> int128_t;
    typedef int128_base<uint64_t, uint64_t> uint128_t;

    template<class _Tp>
    struct half_mask : std::integral_constant<_Tp, (_Tp(1) << (4 * sizeof(_Tp))) - _Tp(1)> {
    };

    template<class, class, class>
    struct float_cast_helper;

    template<class _Tp>
    struct float_cast_helper<_Tp, uint64_t, uint64_t> {
        static constexpr _Tp cast(uint64_t high_, uint64_t low_) { return std::ldexp(_Tp(high_), 64) + _Tp(low_); }
    };

    template<class _Tp>
    struct float_cast_helper<_Tp, int64_t, uint64_t> {
        static constexpr _Tp cast(int64_t high_, uint64_t low_) {
            return high_ < 0 ? -std::ldexp(_Tp(-high_ - (low_ != 0)), 64) - _Tp(-low_)
                             : std::ldexp(_Tp(high_), 64) + _Tp(low_);
        }
    };

    template<bool= true>
    struct detail_delegate;

    constexpr bool operator<(int128_t, int128_t);

    constexpr bool operator<(uint128_t, uint128_t);

    constexpr uint128_t operator>>(uint128_t, int);

    constexpr int128_t operator>>(int128_t, int);

    constexpr int128_t operator*(int128_t, int128_t);

    constexpr uint128_t operator*(uint128_t, uint128_t);

    constexpr uint128_t operator<<(uint128_t, int);

    constexpr int128_t operator<<(int128_t, int);

    inline uint128_t operator/(uint128_t, uint128_t);

    inline int128_t operator/(int128_t, int128_t);

    inline uint128_t operator%(uint128_t, uint128_t);

    inline int128_t operator%(int128_t, int128_t);

    template<class _Hi, class _Low>
    class alignas(sizeof(_Hi) * 2) int128_base final {
        static_assert(sizeof(_Hi) == sizeof(_Low), "low type, high type should have same size");

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        _Low low_{};
        _Hi high_{};
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        _Hi high_{};
        _Low low_{};
#else
#error endian not support
#endif

        constexpr int128_base(_Hi high, _Low low) : high_(high), low_(low) {}

        struct integral_tag {
        };
        struct signed_integral_tag : integral_tag {
        };
        struct unsigned_integral_tag : integral_tag {
        };
        struct float_tag {
        };
        template<size_t>
        struct size_constant {
        };

    private:
        template<class _Tp>
        constexpr int128_base(_Tp value_, signed_integral_tag, size_constant<8>) :
                int128_base(-(value_ < 0), value_) {}

        template<class _Tp>
        constexpr int128_base(_Tp value_, unsigned_integral_tag, size_constant<8>) : int128_base(0, _Low(value_)) {}

        template<class _Tp>
        constexpr int128_base(_Tp value_, integral_tag, size_constant<16>) : // NOLINT explicit
                int128_base(_Hi(value_ >> 64U), _Low(value_)) {} // NOLINT signed shift

    public:

        constexpr int128_base() noexcept = default;

        constexpr int128_base(const int128_base &) noexcept = default;

        constexpr int128_base(int128_base &&) noexcept = default;

        int128_base &operator=(const int128_base &) noexcept = default;

        int128_base &operator=(int128_base &&) noexcept = default;

        template<class _Tp>
        constexpr explicit int128_base(int128_base<_Tp, _Low> v) : int128_base(v.high_, v.low_) {}

        template<class _Tp>
        constexpr int128_base(_Tp value_, float_tag) :
                int128_base(_Hi(std::ldexp(value_, -64)) - (value_ < 0), _Low(value_)) {}

        constexpr explicit int128_base(float value_) : int128_base(value_, float_tag()) {}

        constexpr explicit int128_base(double value_) : int128_base(value_, float_tag()) {}

        constexpr explicit int128_base(long double value_) : int128_base(value_, float_tag()) {}

        constexpr int128_base(long long value_) : // NOLINT explicit
                int128_base(value_, signed_integral_tag(), size_constant<sizeof(value_)>()) {}

        constexpr int128_base(long value_) : int128_base(static_cast<long long>(value_)) {} // NOLINT explicit

        constexpr int128_base(int value_) : int128_base(long(value_)) {} // NOLINT explicit

        constexpr int128_base(unsigned long long value_) : // NOLINT explicit
                int128_base(value_, unsigned_integral_tag(), size_constant<sizeof(value_)>()) {}

        constexpr int128_base(unsigned long value_) : // NOLINT explicit
                int128_base(static_cast<unsigned long long>(value_)) {}

        constexpr int128_base(unsigned value_) : int128_base(static_cast<unsigned long>(value_)) {} // NOLINT explicit

        constexpr explicit operator bool() const { return high_ || low_; }

        constexpr explicit operator char() const { return char(low_); }

        constexpr explicit operator signed char() const { return static_cast<signed char>(low_); }

        constexpr explicit operator unsigned char() const { return static_cast<unsigned char>(low_); }

        constexpr explicit operator short() const { return short(low_); }

        constexpr explicit operator unsigned short() const { return static_cast<unsigned short>(low_); }

        constexpr explicit operator int() const { return int(low_); }

        constexpr explicit operator unsigned() const { return unsigned(low_); }

        constexpr explicit operator long() const { return long(low_); }

        constexpr explicit operator unsigned long() const { return static_cast<unsigned long>(low_); }

        constexpr explicit operator long long() const { return static_cast<long long>(low_); }

        constexpr explicit operator unsigned long long() const { return static_cast<unsigned long long>(low_); }

        constexpr explicit operator wchar_t() const { return wchar_t(low_); }

        constexpr explicit operator char16_t() const { return char16_t(low_); }

        constexpr explicit operator char32_t() const { return char32_t(low_); }

#if __SIZEOF_INT128__ == 16

        constexpr explicit int128_base(__int128 value_) :
                int128_base(value_, signed_integral_tag(), size_constant<sizeof(value_)>()) {}

        constexpr explicit int128_base(unsigned __int128 value_) :
                int128_base(value_, unsigned_integral_tag(), size_constant<sizeof(value_)>()) {}

        constexpr explicit operator unsigned __int128() const {
            return static_cast<unsigned __int128>(high_) << 64U | static_cast<unsigned __int128>(low_);
        }

        constexpr explicit operator __int128() const {
            return static_cast<__int128>(static_cast<unsigned __int128>(*this));
        }

#endif

    private:
        template<class _Tp>
        constexpr _Tp cast_to_float() const { return float_cast_helper<_Tp, _Hi, _Low>::cast(high_, low_); }

    public:
        constexpr explicit operator float() const { return cast_to_float<float>(); }

        constexpr explicit operator double() const { return cast_to_float<double>(); }

        constexpr explicit operator long double() const { return cast_to_float<long double>(); }

        constexpr int128_base operator+() const { return *this; }

        constexpr int128_base operator-() const { return int128_base(-high_ - (low_ != 0), -low_); }

        constexpr int128_base operator~() const { return int128_base(~high_, ~low_); }

        constexpr bool operator!() const { return !high_ && !low_; }

        // avoid self plus on rvalue
        int128_base &operator++() &{ return *this = *this + int128_base(1); }

        int128_base &operator--() &{ return *this = *this - int128_base(1); }

        int128_base operator++(int) &{ // NOLINT returns non constant
            int128_base tmp = *this;
            ++*this;
            return tmp;
        }

        int128_base operator--(int) &{ // NOLINT returns non constant
            int128_base tmp = *this;
            --*this;
            return tmp;
        }

        friend constexpr int128_base operator+(int128_base a, int128_base b) {
            // no worry for unsigned type, won't be optimized if overflow
            return {_Hi(a.high_ + b.high_ + (a.low_ + b.low_ < a.low_)), a.low_ + b.low_};
        }

        friend constexpr int128_base operator-(int128_base a, int128_base b) {
            return {_Hi(a.high_ - b.high_ - (a.low_ < b.low_)), a.low_ - b.low_};
        }

        friend constexpr int128_base operator&(int128_base a, int128_base b) {
            return {a.high_ & b.high_, a.low_ & b.low_};
        }

        friend constexpr int128_base operator|(int128_base a, int128_base b) {
            return {a.high_ | b.high_, a.low_ | b.low_};
        }

        friend constexpr int128_base operator^(int128_base a, int128_base b) {
            return {a.high_ ^ b.high_, a.low_ ^ b.low_};
        }

        friend constexpr bool operator==(int128_base a, int128_base b) {
            return a.high_ == b.high_ && a.low_ == b.low_;
        }

        friend constexpr bool operator>(int128_base a, int128_base b) { return b < a; }

        friend constexpr bool operator>=(int128_base a, int128_base b) { return !(a < b); }

        friend constexpr bool operator<=(int128_base a, int128_base b) { return !(b < a); }

        friend constexpr bool operator!=(int128_base a, int128_base b) { return !(a == b); }

        friend constexpr int128_base operator<<(int128_base x, int128_base y) { return x << (int) y.low_; }

        friend constexpr int128_base operator>>(int128_base x, int128_base y) { return x >> (int) y.low_; }

        int128_base &operator+=(int128_base x) &{ return *this = *this + x; }

        int128_base &operator-=(int128_base x) &{ return *this = *this - x; }

        int128_base &operator*=(int128_base x) &{ return *this = *this * x; }

        int128_base &operator/=(int128_base another) &{ return *this = *this / another; }

        int128_base &operator%=(int128_base another) &{ return *this = *this % another; }

        int128_base &operator<<=(int128_base x) &{ return *this = *this << x; }

        int128_base &operator>>=(int128_base x) &{ return *this = *this >> x; }

        int128_base &operator<<=(int x) &{ return *this = *this << x; }

        int128_base &operator>>=(int x) &{ return *this = *this >> x; }

        int128_base &operator&=(int128_base another) &{ return *this = *this & another; }

        int128_base &operator|=(int128_base another) &{ return *this = *this | another; }

        int128_base &operator^=(int128_base another) &{ return *this = *this ^ another; }

        template<class, class>
        friend
        class int128_base;

        template<class>
        friend
        class clz_helper;

        template<bool>
        friend
        class detail_delegate;
    };

    inline namespace literals {
        namespace impl_ {
            template<char _Ch, int _Rad>
            struct static_digit : std::integral_constant<int,
                    '0' <= _Ch && _Ch <= '9' ? _Ch - '0' :
                    'a' <= _Ch && _Ch <= 'z' ? _Ch - 'a' + 10 :
                    'A' <= _Ch && _Ch <= 'Z' ? _Ch - 'A' + 10 : _Rad> {
                static_assert(_Rad > static_digit::value, "character not a digit");
            };

            template<class, int, char ...>
            struct int128_literal_radix;

            template<class _Tp, int _Rad, char _Ch>
            struct int128_literal_radix<_Tp, _Rad, _Ch> {
                constexpr operator _Tp() const { return _Tp(static_digit<_Ch, _Rad>::value); } // NOLINT explicit

                constexpr _Tp operator()(_Tp v) const { return v * _Tp(_Rad) + *this; }
            };

            template<class _Tp, int _Rad, char _Ch, char ..._Args>
            struct int128_literal_radix<_Tp, _Rad, _Ch, _Args...> {
                int128_literal_radix<_Tp, _Rad, _Ch> _Cur;
                int128_literal_radix<_Tp, _Rad, _Args...> _Tgt;

                constexpr operator _Tp() const { return _Tgt(_Cur); }; // NOLINT explicit

                constexpr _Tp operator()(_Tp v) const { return _Tgt(_Cur(v)); };
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
        }

        template<char ..._Args>
        constexpr uint128_t operator "" _u128() { return impl_::int128_literal<uint128_t, _Args...>(); }

        template<char ..._Args>
        constexpr int128_t operator "" _l128() { return impl_::int128_literal<int128_t, _Args...>(); }

        template<char ..._Args>
        constexpr uint128_t operator "" _U128() { return impl_::int128_literal<uint128_t, _Args...>(); }

        template<char ..._Args>
        constexpr int128_t operator "" _L128() { return impl_::int128_literal<int128_t, _Args...>(); }
    }

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
            return v.high_ ? clz_helper<_Low>::clz(v.high_) : 4 * sizeof(v) + clz_helper<_Low>::clz(v.low_);
        }
    };

    template<bool>
    struct detail_delegate {
        template<class _Hi, class _Low>
        static constexpr bool cmp(int128_base<_Hi, _Low> a, int128_base<_Hi, _Low> b) {
            return a.high_ < b.high_ || (a.high_ == b.high_ && a.low_ < b.low_);
        }

        static constexpr uint128_t shr(uint128_t x, unsigned n) {
            return n & 64U ? uint128_t(0, x.high_ >> (n & 63U)) :
                   n & 63U ? uint128_t(x.high_ >> (n & 63U),
                                       (x.high_ << (64 - (n & 63U)) | (x.low_ >> (n & 63U)))) : x;
        }

        static constexpr int128_t sar(int128_t x, unsigned n) {
            return n & 64U ? int128_t(-(x.high_ < 0), uint64_t(x.high_ >> (n & 63U))) : // NOLINT signed shift
                   n & 63U ? int128_t(x.high_ >> (n & 63U), // NOLINT signed shift
                                      (uint64_t(x.high_) << (64 - (n & 63U)) | (x.low_ >> (n & 63U)))) : x;
        }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low> imul(int128_base<_Hi, _Low> a, int128_base<_Hi, _Low> b) {
            return int128_base<_Hi, _Low>(_Hi(a.low_ * b.high_ + b.low_ * a.high_) + (a.low_ >> 32U) * (b.low_ >> 32U),
                                          (a.low_ & half_mask<_Low>::value) *
                                          (b.low_ & half_mask<_Low>::value))
                   + (int128_base<_Hi, _Low>((a.low_ >> 32U) * (b.low_ & half_mask<_Low>::value)) << 32U)
                   + (int128_base<_Hi, _Low>((b.low_ >> 32U) * (a.low_ & half_mask<_Low>::value)) << 32U);
        }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low> shl(int128_base<_Hi, _Low> x, unsigned n) {
            // [64,127], 64 {low_ << 0, 0}
            return n & 64U ? int128_base<_Hi, _Low>(_Hi(x.low_ << (n & 63U)), _Low(0)) :
                   n & 63U ? int128_base<_Hi, _Low>(_Hi((_Low(x.high_) << (n & 63U)) | (x.low_ >> (64U - (n & 63U)))),
                                                    x.low_ << (n & 63U)) : x;
        }

        static uint128_t &slow_div_(uint128_t &dividend_, uint128_t divisor_, uint128_t &quot_) {
            // assert(divisor != uint128_t(0));
            quot_ = uint128_t(0);
            if (cmp(dividend_, divisor_)) return dividend_;
            if (dividend_.high_ == 0) { // (0,x) / ???
                quot_.low_ = dividend_.low_ / divisor_.low_;
                dividend_.low_ %= divisor_.low_;
                return dividend_;
            }
            auto zend_ = clz_helper<uint128_t>::clz(dividend_), zsor_ = clz_helper<uint128_t>::clz(divisor_);
            if (zend_ > zsor_) return dividend_;
            for (zsor_ -= zend_, divisor_ <<= zsor_;; divisor_ >>= 1, quot_ <<= 1) {
                if (dividend_ >= divisor_) {
                    dividend_ -= divisor_;
                    quot_ |= uint128_t(1);
                }
                if (!zsor_--) return dividend_;
            }
        }

        static uint128_t div(uint128_t dividend_, uint128_t divisor_) {
            if (!divisor_) return {!!dividend_ / !!divisor_}; // raise signal SIGFPE
            uint128_t quot_(0);
            slow_div_(dividend_, divisor_, quot_);
            return quot_;
        }

        static int128_t div(int128_t dividend_, int128_t divisor_) {
            bool nneg_ = dividend_.high_ < 0, dneg_ = divisor_.high_ < 0;
            auto res_ = div(uint128_t(nneg_ ? -dividend_ : dividend_), uint128_t(dneg_ ? -divisor_ : divisor_));
            return int128_t(nneg_ ^ dneg_ ? -res_ : res_);
        }

        static uint128_t mod(uint128_t dividend_, uint128_t divisor_) {
            if (!divisor_) return {!!dividend_ % !!divisor_}; // raise signal SIGFPE
            uint128_t quot_(0);
            return slow_div_(dividend_, divisor_, quot_);
        }

        static int128_t mod(int128_t dividend_, int128_t divisor_) {
            bool neg_ = dividend_.high_ < 0;
            auto res_ = mod(uint128_t(neg_ ? -dividend_ : dividend_),
                            uint128_t(divisor_.high_ < 0 ? -divisor_ : divisor_));
            return int128_t(neg_ ? -res_ : res_);
        }

        static void part_div(uint128_t value_, uint64_t div_, uint64_t &high_, uint64_t &mid_, uint64_t &low_) {
            uint128_t hh_(0), md_(0);
            low_ = static_cast<uint64_t>(slow_div_(value_, div_, md_));
            mid_ = static_cast<uint64_t>(slow_div_(md_, div_, hh_));
            high_ = static_cast<uint64_t>(hh_);
        }
    };

#if __SIZEOF_INT128__ == 16

    template<>
    struct detail_delegate<true> {
        typedef __int128 ti_int_;
        typedef unsigned __int128 tu_int_;

        static constexpr ti_int_ to_native(int128_t x) { return static_cast<ti_int_>(x); }

        static constexpr tu_int_ to_native(uint128_t x) { return static_cast<tu_int_>(x); }

        static constexpr int128_t from_native(ti_int_ x) { return int128_t(x); }

        static constexpr uint128_t from_native(tu_int_ x) { return uint128_t(x); }

        template<class _Hi, class _Low>
        static constexpr bool cmp(int128_base<_Hi, _Low> a, int128_base<_Hi, _Low> b) {
            return to_native(a) < to_native(b);
        }

        static constexpr uint128_t shr(uint128_t x, unsigned n) {
            return from_native(to_native(x) >> static_cast<decltype(to_native(x))>(n));
        }

        static constexpr int128_t sar(int128_t x, unsigned n) {
            return from_native(to_native(x) >> static_cast<decltype(to_native(x))>(n)); // NOLINT signed shift
        }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low> imul(int128_base<_Hi, _Low> a, int128_base<_Hi, _Low> b) {
            return from_native(to_native(a) * to_native(b));
        }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low> shl(int128_base<_Hi, _Low> x, unsigned n) {
            return from_native(to_native(x) << static_cast<decltype(to_native(x))>(n)); // NOLINT signed shift
        }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low>
        div(int128_base<_Hi, _Low> n, int128_base<_Hi, _Low> d) { return from_native(to_native(n) / to_native(d)); }

        template<class _Hi, class _Low>
        static constexpr int128_base<_Hi, _Low>
        mod(int128_base<_Hi, _Low> n, int128_base<_Hi, _Low> d) { return from_native(to_native(n) % to_native(d)); }

        static void part_div(uint128_t value_, uint64_t div_, uint64_t &high_, uint64_t &mid_, uint64_t &low_) {
            // on some cpu, compiler won't do optmize for us
            auto vv_ = to_native(value_);
            auto rest_ = vv_ / div_;
            low_ = static_cast<uint64_t>(vv_) - div_ * static_cast<uint64_t>(rest_);
            high_ = static_cast<uint64_t>(rest_ / div_);
            mid_ = static_cast<uint64_t>(rest_) - div_ * high_;
        }
    };

#endif

    constexpr bool operator<(int128_t a, int128_t b) { return detail_delegate<>::cmp(a, b); }

    constexpr bool operator<(uint128_t a, uint128_t b) { return detail_delegate<>::cmp(a, b); }

    constexpr uint128_t operator>>(uint128_t x, int n) { return detail_delegate<>::shr(x, static_cast<unsigned>(n)); }

    constexpr int128_t operator>>(int128_t x, int n) { return detail_delegate<>::sar(x, static_cast<unsigned>(n)); }

    constexpr int128_t operator*(int128_t a, int128_t b) { return detail_delegate<>::imul(a, b); }

    constexpr uint128_t operator*(uint128_t a, uint128_t b) { return detail_delegate<>::imul(a, b); }

    constexpr uint128_t operator<<(uint128_t x, int n) { return detail_delegate<>::shl(x, static_cast<unsigned>(n)); }

    constexpr int128_t operator<<(int128_t x, int n) { return detail_delegate<>::shl(x, static_cast<unsigned>(n)); }

    inline uint128_t operator/(uint128_t n, uint128_t d) { return detail_delegate<>::div(n, d); };

    inline int128_t operator/(int128_t n, int128_t d) { return detail_delegate<>::div(n, d); };

    inline uint128_t operator%(uint128_t n, uint128_t d) { return detail_delegate<>::mod(n, d); };

    inline int128_t operator%(int128_t n, int128_t d) { return detail_delegate<>::mod(n, d); }

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &
    print_value(std::basic_ostream<_CharT, _Traits> &out_, bool signed_integral_, uint128_t value_) {
        constexpr std::size_t buf_size_ = 45;

        typename std::basic_ostream<_CharT, _Traits>::sentry sentry_(out_);
        if (!sentry_) return out_;
        auto flags_ = out_.flags(), base_flag_ = flags_ & std::ios::basefield;
        auto adjust_field_ = flags_ & std::ios::adjustfield;
        auto show_base_ = bool(flags_ & std::ios::showbase); // work not oct
        auto show_pos_ = bool(flags_ & std::ios::showpos); // work only oct
        auto upper_case_ = bool(flags_ & std::ios::uppercase); // work only hex
        auto ns_ = out_.width(0);
        auto fl_ = out_.fill();

        char buf_[buf_size_];
        char const *prefix_ = nullptr;
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
                constexpr uint64_t mask_ = (UINT64_C(1) << 63U) - 1;
                if (show_base_ && value_) buf_[offset_++] = '0';
                auto x_ = (uint64_t) (value_ >> 126U);
                auto y_ = (uint64_t) (value_ >> 63U) & mask_;
                auto z_ = (uint64_t) (value_) & mask_;
                if (x_) {
                    offset_ += snprintf(buf_ + offset_, buf_size_ - offset_, "%" PRIo64 "%021" PRIo64 "%021" PRIo64,
                                        x_, y_, z_);
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
                uint64_t high_, mid_, low_;
                detail_delegate<>::part_div(value_, UINT64_C(10000000000000000000), high_, mid_, low_);
                if (high_) {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64 "%019" PRIu64 "%019" PRIu64,
                                       high_, mid_, low_);
                } else if (mid_) {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64 "%019" PRIu64,
                                       mid_, low_);
                } else {
                    offset_ = snprintf(buf_, buf_size_, "%" PRIu64, low_);
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

        // no worry group is not empty
        auto limit_ = grouping_.size();
        if (limit_ == 0) {
            op_ = oe_ - offset_;
            ct_.widen(buf_, buf_ + offset_, op_);
        } else {
            auto thousands_sep_ = npt_.thousands_sep();
            decltype(limit_) dg_ = 0;
            auto cnt_ = static_cast<unsigned char>(grouping_[dg_]);
            unsigned char dc_ = 0;
            --limit_;
            op_ = oe_;
            for (char *p_ = buf_ + offset_; p_ != buf_; ++dc_) {
                if (cnt_ > 0 && dc_ == cnt_) {
                    *--op_ = thousands_sep_;
                    dc_ = 0;
                    if (dg_ < limit_) cnt_ = static_cast<unsigned char>(grouping_[++dg_]);
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
                default:
                    return out_.write(sp_.data(), ns_).write(os_, sz_);
            }
        }
        return out_.write(os_, sz_);
    }

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &out, uint128_t _Val) {
        return print_value(out, false, _Val);
    }

    template<class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &out, int128_t _Val) {
        return print_value(out, true, uint128_t(_Val));
    }
}

#ifdef INT128_SPECIALIZATION
namespace std {
#pragma push_macro("MAKE_TYPE")
#define MAKE_TYPE(outter, inner, parent) \
template<> struct outter<large_int::inner> : std::parent {}; \
template<> struct outter<const large_int::inner> : std::parent {}; \
template<> struct outter<volatile large_int::inner> : std::parent {}; \
template<> struct outter<const volatile large_int::inner> : std::parent {};
    MAKE_TYPE(is_integral, uint128_t, true_type)
    MAKE_TYPE(is_integral, int128_t, true_type)
    MAKE_TYPE(is_signed, uint128_t, false_type)
    MAKE_TYPE(is_signed, int128_t, true_type)
#undef MAKE_TYPE
#define MAKE_TYPE(outter, inner, target) \
template<> struct outter<large_int::inner> { typedef large_int::target type; }; \
template<> struct outter<const large_int::inner> { typedef const large_int::target type; }; \
template<> struct outter<volatile large_int::inner> { typedef volatile large_int::target type; }; \
template<> struct outter<const volatile large_int::inner> { typedef const volatile large_int::target type; };
    MAKE_TYPE(make_signed, uint128_t, int128_t)
    MAKE_TYPE(make_unsigned, int128_t, uint128_t)
#pragma pop_macro("MAKE_TYPE")
    template<class _Hi, class _Low>
    struct numeric_limits<large_int::int128_base<_Hi, _Low> > {
    private:
        typedef large_int::int128_base<_Hi, _Low>_Tp;
    public:
        static constexpr const bool is_specialized = true;
        static constexpr const bool is_signed = numeric_limits<_Hi>::is_signed;
        static constexpr const bool is_integer = true;
        static constexpr const bool is_exact = true;
        static constexpr const bool has_infinity = false;
        static constexpr const bool has_quiet_NaN = false;
        static constexpr const bool has_signaling_NaN = false;
        static constexpr const std::float_denorm_style has_denorm = std::denorm_absent;
        static constexpr const bool has_denorm_loss = false;
        static constexpr const std::float_round_style round_style = std::round_toward_zero;
        static constexpr const bool is_iec559 = false;
        static constexpr const bool is_bounded = true;
        static constexpr const bool is_modulo = numeric_limits<_Hi>::is_modulo;
        static constexpr const int digits = static_cast<int>(sizeof(_Tp) * 8 - is_signed);
        static constexpr const int digits10 = digits * 3 / 10;
        static constexpr const int max_digits10 = 0;
        static constexpr const int radix = 2;
        static constexpr const int min_exponent = 0;
        static constexpr const int min_exponent10 = 0;
        static constexpr const int max_exponent = 0;
        static constexpr const int max_exponent10 = 0;
        static constexpr const bool traps = numeric_limits<_Hi>::traps;
        static constexpr const bool tinyness_before = false;

        static constexpr _Tp min() { return is_signed ? _Tp(1) << digits : _Tp(0); }

        static constexpr _Tp lowest() { return min(); }

        static constexpr _Tp max() { return ~min(); }

        static constexpr _Tp epsilon() { return _Tp(0); }

        static constexpr _Tp round_error() { return _Tp(0); }

        static constexpr _Tp infinity() { return _Tp(0); }

        static constexpr _Tp quiet_NaN() { return _Tp(0); }

        static constexpr _Tp signaling_NaN() { return _Tp(0); }

        static constexpr _Tp denorm_min() { return _Tp(0); }
    };
}
#endif /* INT128_SPECIALIZATION */

#ifndef INT128_NO_EXPORT
#define INT128_C(val) val##_L128
#define UINT128_C(val) val##_U128
// add space between ‘""’ and suffix identifier, or may compile failed
using namespace large_int::literals;
using large_int::uint128_t;
using large_int::int128_t;
#endif /* INT128_NO_EXPORT */
