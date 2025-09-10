#pragma once

#define BITFIELD_DATA_UINT64(initial_value) \
	using underlying_type = uint64_t; \
	underlying_type v = initial_value;


#define BITFIELD_FIELD_SHIFT(name) name ## _SHIFT
#define BITFIELD_FIELD_MASK(name) name ## _MASK


#define BITFIELD_FIELD_CONSTANTS(name, offset, width) \
	static constexpr underlying_type BITFIELD_FIELD_SHIFT(name) = offset; \
	static constexpr underlying_type BITFIELD_FIELD_MASK(name) = (underlying_type(1) << width) - 1;


#define BITFIELD_FIELD_GETTER(type, name) \
	constexpr type name() const { \
		return static_cast<type>((v >> BITFIELD_FIELD_SHIFT(name)) & BITFIELD_FIELD_MASK(name)); \
	}


#define BITFIELD_FIELD_SETTER(type, name) \
	constexpr void set_ ## name(type new_value) { \
		v &= ~(BITFIELD_FIELD_MASK(name) << BITFIELD_FIELD_SHIFT(name)); \
		v |= (static_cast<underlying_type>(new_value) & BITFIELD_FIELD_MASK(name)) << BITFIELD_FIELD_SHIFT(name); \
	} \
	\
	constexpr auto with_ ## name(type new_value) const { \
		auto copy = *this; \
		copy.set_ ## name(new_value); \
		return copy; \
	}


#define BITFIELD_FIELD_RW(type, name, offset, width) \
	BITFIELD_FIELD_CONSTANTS(name, offset, width) \
	BITFIELD_FIELD_GETTER(type, name) \
	BITFIELD_FIELD_SETTER(type, name)


#define BITFIELD_FIELD_RO(type, name, offset, width) \
	BITFIELD_FIELD_CONSTANTS(name, offset, width) \
	BITFIELD_FIELD_GETTER(type, name)
