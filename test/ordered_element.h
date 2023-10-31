#pragma once

#include <vector>

class ordered_element {
public:
  ordered_element(size_t val): val(val) {
    insertion_order().push_back(val);
  }

  ~ordered_element() {
    size_t back = insertion_order().back();
    ASSERT_EQ(val, back) << "Elements must be destroyed in order reversed to insertion";
    insertion_order().pop_back();
  }

  static std::vector<size_t>& insertion_order() {
    static std::vector<size_t> insertion_order;
    return insertion_order;
  }

private:
  size_t val;
};
