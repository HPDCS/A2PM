#pragma once

#include <stdbool.h>


//#define SPINLOCK_GIVES_COUNT



/// Atomic counter definition
typedef struct { volatile int count; } atomic_t;

/// Spinlock definition
typedef struct { volatile unsigned int lock; } spinlock_t;



inline bool CAS_x86(volatile unsigned long long *ptr, unsigned long long oldVal, unsigned long long newVal);
inline bool iCAS_x86(volatile unsigned int *ptr, unsigned int oldVal, unsigned int newVal);
inline int atomic_test_and_set_x86(int *);
inline int atomic_test_and_reset_x86(int *);
inline void atomic_add_x86(atomic_t *, int);
inline void atomic_sub_x86(atomic_t *, int);
inline void atomic_inc_x86(atomic_t *);
inline void atomic_dec_x86(atomic_t *);
inline int atomic_inc_and_test_x86(atomic_t *v);
inline bool spin_trylock_x86(spinlock_t *s);
inline void spin_unlock_x86(spinlock_t *s);

#ifdef SPINLOCK_GIVES_COUNT
inline unsigned int spin_lock_x86(spinlock_t *s);
#else
inline void spin_lock_x86(spinlock_t *s);
#endif


#define CAS			CAS_x86
#define iCAS			iCAS_x86
#define atomic_test_and_set	atomic_test_and_set_x86
#define atomic_test_and_reset	atomic_test_and_reset_x86
#define atomic_add		atomic_add_x86
#define atomic_sub		atomic_sub_x86
#define atomic_dec		atomic_dec_x86
#define atomic_inc		atomic_inc_x86
#define atomic_inc_and_test	atomic_inc_and_test_x86
#define spin_lock		spin_lock_x86
#define spin_trylock		spin_trylock_x86
#define spin_unlock		spin_unlock_x86

#define LOCK "lock; "


/// Read operation on an atomic counter
#define atomic_read(v)		((v)->count)

/// Set operation on an atomic counter
#define atomic_set(v,i)		(((v)->count) = (i))

/// Spinlock initialization
#define spinlock_init(s)	((s)->lock = 0)




