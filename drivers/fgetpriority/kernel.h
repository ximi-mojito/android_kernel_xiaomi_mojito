
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#pragma once
// gcc -std=gnu23 -dM -E -x c /dev/null
// NOTE: gcc14 uses 202000L on -std=gnu23
#if (defined(__clang__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || \
	(!defined(__clang__) && (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202000L))
#define COMPILER_HAS_C23
#endif

#ifdef COMPILER_HAS_C23
#define bool  __fgp_bool
#define false __fgp_false
#define true  __fgp_true
#include <linux/types.h>
#include <linux/stddef.h>
#undef false
#undef true
#undef bool
#endif // COMPILER_HAS_C23

#include <asm/current.h>
#include <asm/memory.h>
#include <asm/syscall.h>

#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/ioprio.h>
#include <linux/module.h>
#include <linux/resource.h>
#include <linux/string.h>
#include <linux/thread_info.h>
#include <linux/stop_machine.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#ifndef __ro_after_init
#define __ro_after_init
#endif

#ifndef __nocfi
#define __nocfi
#endif

#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#ifndef __has_feature
#define __has_feature(x) (0)
#endif

#ifndef __has_c_attribute
#define __has_c_attribute(x) (0)
#endif

#ifndef __has_include
#define __has_include(x) (0)
#endif

#ifndef __has_extension
#define __has_extension(x) (0)
#endif

#ifndef __has_attribute
#define __has_attribute(x) (0)
#endif

/**
 * Linux kernel restricts C99 restrict
 * however we can use builtin's restrict
 */
#define restrict __restrict

/**
 * partially emulate-able C23 features, should be fine on GNU11 compilers
 *
 * Limitations:
 *	- do NOT use nullptr_t on _Generic overloading, it will fuck up on C11
 *	- do NOT use constexpr as array size on C11, it will likely become a VLA
 */
#if !defined(COMPILER_HAS_C23)
#define nullptr ((void *)0)
typedef typeof(nullptr) nullptr_t;
#define constexpr const
#define auto __auto_type
#define alignas _Alignas
#define alignof _Alignof
#endif // COMPILER_HAS_C23

// NOTE: clang < 19 has issues on constexpr even with -std=gnu23
#if defined (COMPILER_HAS_C23) && defined(__clang__) && (__clang_major__ < 19)
#define constexpr const
#endif

/**
 * typeof_unqual requires C23, we import kernel's typeof_unqual for !C23
 * https://elixir.bootlin.com/linux/v7.3-rc4/source/include/linux/compiler_types.h#L616
 *
 */
#if !defined(COMPILER_HAS_C23)
#define __scalar_type_to_expr_cases(type)	\
	unsigned type:	(unsigned type)0,	\
	signed type:	(signed type)0

#define typeof_unqual(x) typeof(				\
	_Generic((x),						\
		char:	(char)0,				\
		__scalar_type_to_expr_cases(char),		\
		__scalar_type_to_expr_cases(short),		\
		__scalar_type_to_expr_cases(int),		\
		__scalar_type_to_expr_cases(long),		\
		__scalar_type_to_expr_cases(long long),		\
		default: (x)))
#endif

/**
 * static_assert is C23
 * this has an alternative available on C11 capable compilers.
 * ref: https://elixir.bootlin.com/linux/v5.1/source/include/linux/build_bug.h
 *
 * static_assert(condition); - condition becomes the comment
 * static_assert(condition, "comment");
 */
#if !defined(COMPILER_HAS_C23) && !defined(static_assert)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)
#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#endif
#if defined(COMPILER_HAS_C23) && defined(static_assert) // undef kernel's static_assert
#undef static_assert
#endif

/**
 * nodiscard is C23, we can fallback to __must_check
 */
#if __has_c_attribute(nodiscard)
#define nodiscard [[nodiscard]]
#else
#define nodiscard __attribute__((__warn_unused_result__))
#endif

/**
 * old compilers does NOT know fallthrough, this is GNU/C23
 * however we can use a comment and it silences it (implicit fallthrough)
 * ref: https://elixir.bootlin.com/linux/v7.2.2/source/include/linux/compiler_attributes.h#L216
 */
#ifndef fallthrough
#if __has_c_attribute(fallthrough)
#define fallthrough [[fallthrough]]
#elif __has_attribute(__fallthrough__) || defined(__clang__)
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough do { } while (0) /* fallthrough */
#endif
#endif

/**
 * expr assumptions for micro-optimizations
 */
#if defined(__clang__)
#define assume(expr) __builtin_assume(expr)
#elif defined(__GNUC__) && (__GNUC__ >= 13)
#define assume(expr) __attribute__((assume(expr)))
#else
#define assume(expr) do {			\
	if (unlikely(!(expr)))			\
		__builtin_unreachable();	\
} while (0)
#endif

/**
 * *ptr + sizeof(*ptr) assumptions for micro-optimizations
 */
#if defined(__clang__) && __has_builtin(__builtin_assume_dereferenceable)
#define assume_dereferenceable(ptr, size) __builtin_assume_dereferenceable(ptr, size)
#else
#define assume_dereferenceable(ptr, size) do { } while (0)
#endif

/**
 * restriction assumptions for micro-optimizations
 */
#if defined(__clang__) && __has_builtin(__builtin_assume_separate_storage)
#define assume_restricted(ptr1, ptr2) __builtin_assume_separate_storage(ptr1, ptr2)
#else
#define assume_restricted(ptr1, ptr2) do { } while (0)
#endif

/**
 * we do NOT have memset_explicit on the linux kernel
 *
 * from: OPENSSL_cleanse, volatile function pointer prevents memset optimization
 * https://github.com/openssl/openssl/blob/master/crypto/mem_clr.c
 * 
 */
static __nocfi __always_inline void *memset_explicit(void *s, int c, size_t count)
{
	static typeof(memset) *volatile memset_fnptr = memset;
	return memset_fnptr(s, c, count);
}

/**
 * C2y's countof
 * 
 * - this is literally like kernel's ARRAY_SIZE
 */
#if __has_feature(c_countof) || __has_extension(c_countof)
#define countof(a) _Countof(a)
#else
#define countof(a) (sizeof(a) / sizeof(a[0]))
#endif

/**
 * uint128_t / int128_t
 *
 * - nonstandard, this exists as an extension on gcc and clang
 * - can be used with atomics on arm64 via ldxp+stxp or LSE / LSE2, no neon entry required.
 *
 */
#if defined(CONFIG_64BIT) && defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ == 16)
#define COMPILER_HAS_INT128
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
#define make128const(hi,lo) ((((int128_t)hi << 64) | lo))
#endif

/**
 * memcmp_inline / memcpy_inline / memset_inline
 *
 * - guaranteed inline builtin routines
 * - https://github.com/llvm/llvm-project/blob/main/libc/docs/dev/builtin_compatibility.md
 * - fallback to builtin + assert for constexpr sizes
 *
 * NOTE:
 * 	- IR generation tends to fail on older clang, we lock this to 17+
 */
#if __has_builtin(__builtin_memcmp_inline) && defined(__clang__) && (__clang_major__ >= 17)
#define memcmp_inline	__builtin_memcmp_inline
#else
#define memcmp_inline(cs, ct, count) ({			\
	static_assert(__builtin_constant_p(count));	\
	__builtin_memcmp((cs), (ct), (count));		\
})
#endif

#if __has_builtin(__builtin_memcpy_inline) && defined(__clang__) && (__clang_major__ >= 17)
#define memcpy_inline	__builtin_memcpy_inline
#else
#define memcpy_inline(to, from, sz) ({			\
	static_assert(__builtin_constant_p(sz));	\
	__builtin_memcpy((to), (from), (sz));		\
})
#endif

#if __has_builtin(__builtin_memset_inline) && defined(__clang__) && (__clang_major__ >= 17)
#define memset_inline	__builtin_memset_inline
#else
#define memset_inline(dst, val, sz) ({			\
	static_assert(__builtin_constant_p(sz));	\
	__builtin_memset((dst), (val), (sz));		\
})
#endif

/**
 * __may_alias to workaround "optimizations" even on -fno-strict-aliasing
 */
#define __may_alias __attribute__((__may_alias__))

/**
 * __attribute__((__cleanup__(fn)))
 * - pseudo-raii / defer / scoped cleanup on C 
 *
 * NOTE: passes address of variable attributed to fn()
 */
#ifndef __cleanup
#define __cleanup(fn) __attribute__((__cleanup__(fn)))
#endif

// dummy variable generator
#define __fgp_concat(a, b) a##b
#define __fgp_generate_dummy(a, b) __fgp_concat(a, b)
#define __fgp_dummy_var __fgp_generate_dummy(_fgp_dummy_, __COUNTER__)

// scoped lock, mutex
static inline void mutex_unlock_byref(struct mutex **m) { mutex_unlock(*m); }
#define deferred_mutex_unlock(lock) struct mutex *__fgp_dummy_var __cleanup(mutex_unlock_byref) = (lock)
#define guarded_mutex_lock(lock) ({ mutex_lock(lock); deferred_mutex_unlock(lock); 1; })

// scoped lock, spinlock
static inline void spin_unlock_byref(spinlock_t **lock) { spin_unlock(*lock); }
#define deferred_spin_unlock(lock) spinlock_t *__fgp_dummy_var __cleanup(spin_unlock_byref) = (lock)
#define guarded_spin_lock(lock) ({ spin_lock(lock); deferred_spin_unlock(lock); 1; })

// scoped allocations and basic stack offload.
static inline void kfree_byref(void *buf) { kfree(*(void **)buf); }
#define __scoped_kmalloc(size, flags)	__cleanup(kfree_byref) = kmalloc(size, flags)
#define __offstack_flags(size, flags)	__scoped_kmalloc(size, flags)
#define __offstack(size)		__scoped_kmalloc(size, GFP_KERNEL | __GFP_NOFAIL)
#define __zoffstack(size)		__scoped_kmalloc(size, GFP_KERNEL | __GFP_NOFAIL | __GFP_ZERO)

