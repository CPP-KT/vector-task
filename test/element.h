#pragma once

#include <cstddef>
#include <set>

namespace ct::test {

struct Element {
  struct NoNewInstancesGuard;

  Element() = delete;
  Element(int data); // NOLINT
  Element(const Element& other);
  Element(Element&& other);
  ~Element();

  Element& operator=(const Element& c);
  Element& operator=(Element&& c);
  operator int() const; // NOLINT

  static void reset_counters();
  static size_t get_copy_counter();
  static size_t get_move_counter();

  friend void swap(Element&, Element&); // NOLINT

private:
  void add_instance();
  void delete_instance();
  void assert_exists() const;

private:
  int data;

  static std::set<const Element*> instances;
  inline static size_t copy_counter = 0;
  inline static size_t move_counter = 0;
};

struct Element::NoNewInstancesGuard {
  NoNewInstancesGuard();

  NoNewInstancesGuard(const NoNewInstancesGuard&) = delete;
  NoNewInstancesGuard& operator=(const NoNewInstancesGuard&) = delete;

  ~NoNewInstancesGuard();

  void expect_no_instances();

private:
  std::set<const Element*> old_instances;
};

struct ElementWithNonThrowingMove : Element {
  using Element::Element;

  ElementWithNonThrowingMove(const ElementWithNonThrowingMove& other) noexcept = default;
  ElementWithNonThrowingMove(ElementWithNonThrowingMove&& other) noexcept = default;

  ElementWithNonThrowingMove& operator=(const ElementWithNonThrowingMove& other) = default;
  ElementWithNonThrowingMove& operator=(ElementWithNonThrowingMove&& other) noexcept = default;
};
} // namespace ct::test
