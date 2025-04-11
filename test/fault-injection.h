#pragma once

#include <functional>
#include <stdexcept>

namespace ct::test {

struct InjectedFault : std::runtime_error {
  using runtime_error::runtime_error;
};

bool should_inject_fault();
bool move_throw_disabled();
bool fault_injection_point();
void faulty_run(const std::function<void()>& f);

struct FaultInjectionDisable {
  FaultInjectionDisable();

  void reset() const;

  FaultInjectionDisable(const FaultInjectionDisable&) = delete;
  FaultInjectionDisable& operator=(const FaultInjectionDisable&) = delete;

  ~FaultInjectionDisable();

private:
  bool was_disabled;
};

struct FaultInjectionMoveThrowDisable {
  FaultInjectionMoveThrowDisable();

  void reset() const;

  FaultInjectionMoveThrowDisable(const FaultInjectionMoveThrowDisable&) = delete;
  FaultInjectionMoveThrowDisable& operator=(const FaultInjectionMoveThrowDisable&) = delete;

  ~FaultInjectionMoveThrowDisable();

private:
  bool was_enabled;
};

} // namespace ct::test
