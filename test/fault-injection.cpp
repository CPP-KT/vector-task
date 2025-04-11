#include "fault-injection.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace ct::test {

void* injected_allocate(size_t count) {
  if (should_inject_fault()) {
    throw std::bad_alloc();
  }

  void* ptr = malloc(count); // NOLINT
  if (ptr == nullptr) {
    throw std::bad_alloc();
  }

  return ptr;
}

void injected_deallocate(void* ptr) {
  free(ptr); // NOLINT
}

template <typename T>
struct FaultInjectionAllocator {
  using value_type = T;

  FaultInjectionAllocator() = default;

  template <typename U>
  FaultInjectionAllocator(const FaultInjectionAllocator<U>&) {} // NOLINT

  template <typename U>
  FaultInjectionAllocator& operator=(const FaultInjectionAllocator<U>&) {} // NOLINT

  T* allocate(size_t count) {
    return static_cast<T*>(injected_allocate(count * sizeof(T)));
  }

  void deallocate(void* ptr, size_t /*sz*/) {
    injected_deallocate(ptr);
  }
};

struct FaultInjectionContext {
  std::vector<size_t, FaultInjectionAllocator<size_t>> skip_ranges;
  size_t error_index = 0;
  size_t skip_index = 0;
  bool fault_registered = false;
};

thread_local bool disabled = false;
thread_local bool _move_throw_disabled = false;

thread_local FaultInjectionContext* context = nullptr;

void dump_state() {
#if 0 // NOLINT
  FaultInjectionDisable dg;
  std::cout << "skip_ranges: {";
  if (!context->skip_ranges.empty()) {
    std::cout << context->skip_ranges[0];
    for (size_t i = 1; i != context->skip_ranges.size(); ++i) {
      std::cout << ", " << context->skip_ranges[i];
    }
  }
  std::cout << "}\nerror_index: " << context->error_index << "\nskip_index: " << context->skip_index << '\n'
            << std::flush;
#endif
}

bool should_inject_fault() {
  if (context == nullptr) {
    return false;
  }

  if (disabled) {
    return false;
  }

  assert(context->error_index <= context->skip_ranges.size());
  if (context->error_index == context->skip_ranges.size()) {
    FaultInjectionDisable dg;
    ++context->error_index;
    context->skip_ranges.push_back(0);
    context->fault_registered = true;
    return true;
  }

  assert(context->skip_index <= context->skip_ranges[context->error_index]);

  if (context->skip_index == context->skip_ranges[context->error_index]) {
    ++context->error_index;
    context->skip_index = 0;
    context->fault_registered = true;
    return true;
  }

  ++context->skip_index;
  return false;
}

bool move_throw_disabled() {
  return _move_throw_disabled;
}

[[maybe_unused]] bool fault_injection_point() {
  if (should_inject_fault()) {
    FaultInjectionDisable dg;
    throw InjectedFault("injected fault");
  }
  return true;
}

void faulty_run(const std::function<void()>& f) {
  assert(!context);
  FaultInjectionContext ctx;
  context = &ctx;
  for (;;) {
    try {
      f();
    } catch (...) {
      FaultInjectionDisable dg;
      dump_state();
      ctx.skip_ranges.resize(ctx.error_index);
      ++ctx.skip_ranges.back();
      ctx.error_index = 0;
      ctx.skip_index = 0;
      assert(ctx.fault_registered);
      ctx.fault_registered = false;
      continue;
    }
    assert(!ctx.fault_registered);
    break;
  }
  context = nullptr;
}

FaultInjectionDisable::FaultInjectionDisable()
    : was_disabled(disabled) {
  disabled = true;
}

void FaultInjectionDisable::reset() const {
  disabled = was_disabled;
}

FaultInjectionDisable::~FaultInjectionDisable() {
  reset();
}

FaultInjectionMoveThrowDisable::FaultInjectionMoveThrowDisable()
    : was_enabled(_move_throw_disabled) {
  _move_throw_disabled = true;
}

void FaultInjectionMoveThrowDisable::reset() const {
  _move_throw_disabled = was_enabled;
}

FaultInjectionMoveThrowDisable::~FaultInjectionMoveThrowDisable() {
  reset();
}

} // namespace ct::test

void* operator new(size_t count) {
  return ct::test::injected_allocate(count);
}

void* operator new(size_t count, std::align_val_t /*al*/) {
  return ct::test::injected_allocate(count);
}

void* operator new[](size_t count) {
  return ct::test::injected_allocate(count);
}

void operator delete(void* ptr) noexcept {
  ct::test::injected_deallocate(ptr);
}

void operator delete(void* ptr, std::align_val_t /*al*/) noexcept {
  ct::test::injected_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
  ct::test::injected_deallocate(ptr);
}

void operator delete(void* ptr, size_t /*sz*/) noexcept {
  ct::test::injected_deallocate(ptr);
}

void operator delete[](void* ptr, size_t /*sz*/) noexcept {
  ct::test::injected_deallocate(ptr);
}
