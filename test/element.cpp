#include "element.h"

#include "fault-injection.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <utility>

namespace ct::test {

Element::Element(int data)
    : data(data) {
  fault_injection_point();
  add_instance();
}

Element::Element(const Element& other)
    : data(other.data) {
  fault_injection_point();
  add_instance();
  ++copy_counter;
}

Element::Element(Element&& other)
    : data(std::exchange(other.data, -1)) {
  [[maybe_unused]] auto val = move_throw_disabled() || fault_injection_point();
  add_instance();
  ++move_counter;
}

Element::~Element() {
  delete_instance();
}

Element& Element::operator=(const Element& c) { // NOLINT
  assert_exists();
  fault_injection_point();

  ++copy_counter;
  data = c.data;
  return *this;
}

Element& Element::operator=(Element&& c) {
  assert_exists();
  [[maybe_unused]] auto val = move_throw_disabled() || fault_injection_point();

  ++move_counter;
  data = std::exchange(c.data, -1);
  return *this;
}

Element::operator int() const {
  assert_exists();
  fault_injection_point();

  return data;
}

void swap(Element& lhs, Element& rhs) { // NOLINT
  [[maybe_unused]] auto val = move_throw_disabled() || fault_injection_point();
  std::swap(lhs.data, rhs.data);
}

void Element::reset_counters() {
  copy_counter = 0;
  move_counter = 0;
}

size_t Element::get_copy_counter() {
  return copy_counter;
}

size_t Element::get_move_counter() {
  return move_counter;
}

void Element::add_instance() {
  FaultInjectionDisable dg;
  auto p = instances.insert(this);
  if (!p.second) {
    std::stringstream ss;
    ss << "A new object is created at the address " << static_cast<void*>(this)
       << " while the previous object at this address was not destroyed";
    throw std::logic_error(ss.str());
  }
}

void Element::delete_instance() {
  FaultInjectionDisable dg;
  size_t erased = instances.erase(this);
  if (erased != 1) {
    std::stringstream ss;
    ss << "Attempt of destroying non-existing object at address " << static_cast<void*>(this) << '\n';
    FAIL(ss.str());
  }
}

void Element::assert_exists() const {
  FaultInjectionDisable dg;
  bool exists = instances.find(this) != instances.end();
  if (!exists) {
    std::stringstream ss;
    ss << "Accessing a non-existing object at address " << static_cast<const void*>(this);
    throw std::logic_error(ss.str());
  }
}

std::set<const Element*> Element::instances;

Element::NoNewInstancesGuard::NoNewInstancesGuard()
    : old_instances(instances) {}

Element::NoNewInstancesGuard::~NoNewInstancesGuard() {
  CHECK(old_instances == instances);
}

void Element::NoNewInstancesGuard::expect_no_instances() {
  CHECK(old_instances == instances);
}

} // namespace ct::test
