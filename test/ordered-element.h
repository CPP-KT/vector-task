#pragma once

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <vector>

namespace ct::test {

class OrderedElement {
public:
  OrderedElement(size_t val) // NOLINT
      : val(val) {
    insertion_order().push_back(val);
  }

  OrderedElement(const OrderedElement& other)
      : val(other.val) {
    other.val = 0; // NOLINT
  }

  ~OrderedElement() {
    delete_instance();
  }

  static std::vector<size_t>& insertion_order() {
    static std::vector<size_t> insertion_order;
    return insertion_order;
  }

private:
  mutable size_t val;

  void delete_instance() const {
    if (val == 0) {
      return;
    }

    size_t back = insertion_order().back();
    INFO("Elements must be destroyed in reverse order of insertion");
    REQUIRE(val == back);
    insertion_order().pop_back();
  }
};

} // namespace ct::test
