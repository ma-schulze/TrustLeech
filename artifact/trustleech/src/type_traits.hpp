#pragma once

template<typename T, T v>
struct integral_constant {
	static constexpr T value = v;
};

template<bool B>
using bool_constant = integral_constant<bool, B>;

template<typename T>
struct is_standard_layout : bool_constant<__is_standard_layout(T)> {};

template<typename T>
inline constexpr bool is_standard_layout_v = is_standard_layout<T>::value;
