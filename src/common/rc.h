#pragma once

#include <common/defines.h>
#include <common/checker.h>

typedef struct {
    isize count;
} RefCount;

void _increment_rc(RefCount*);
bool _decrement_rc(RefCount*);

// initialize reference count to zero.
void init_rc(RefCount*);
// atomic increment reference count by one.
#define increment_rc(checker, rc) checker_begin_ctx_before_call(checker, _increment_rc, rc)

// atomic decrement reference count by one. Returns true if reference
// count becomes zero or below.
#define decrement_rc(checker, rc) checker_end_ctx_after_call(checker, _decrement_rc, rc)
