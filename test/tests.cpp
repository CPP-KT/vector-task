#include "element.h"
#include "fault-injection.h"
#include "ordered-element.h"
#include "vector.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <utility>

namespace ct {
template class Vector<int>;
template class Vector<test::Element>;
template class Vector<std::string>;
template class Vector<test::OrderedElement>;

namespace test {

template <class Actual, class Expected>
void expect_eq(const Actual& actual, const Expected& expected) {
  FaultInjectionDisable dg;

  CHECK(expected.size() == actual.size());

  if (!std::equal(expected.begin(), expected.end(), actual.begin(), actual.end())) {
    std::stringstream out;
    out << '{';

    bool add_comma = false;
    for (const auto& e : expected) {
      if (add_comma) {
        out << ", ";
      }
      out << e;
      add_comma = true;
    }

    out << "} != {";

    add_comma = false;
    for (const auto& e : actual) {
      if (add_comma) {
        out << ", ";
      }
      out << e;
      add_comma = true;
    }

    out << "}\n";

    FAIL(out.str());
  }
}

template <typename C>
class StrongExceptionSafetyGuard {
public:
  explicit StrongExceptionSafetyGuard(const C& c) noexcept
      : ref(c)
      , expected((FaultInjectionDisable{}, c)) {}

  StrongExceptionSafetyGuard(const StrongExceptionSafetyGuard&) = delete;

  ~StrongExceptionSafetyGuard() {
    if (std::uncaught_exceptions() > 0) {
      expect_eq(expected, ref);
    }
  }

private:
  const C& ref;
  C expected;
};

template <>
class StrongExceptionSafetyGuard<Element> {
public:
  explicit StrongExceptionSafetyGuard(const Element& c) noexcept
      : ref(c)
      , expected((FaultInjectionDisable{}, c)) {}

  StrongExceptionSafetyGuard(const StrongExceptionSafetyGuard&) = delete;

  ~StrongExceptionSafetyGuard() {
    if (std::uncaught_exceptions() > 0) {
      do_assertion();
    }
  }

private:
  void do_assertion() {
    FaultInjectionDisable dg;
    REQUIRE(expected == ref);
  }

private:
  const Element& ref;
  Element expected;
};

class BaseTest {
protected:
  BaseTest() {
    OrderedElement::insertion_order().clear();
  }

  void expect_empty_storage(const Vector<Element>& a) {
    instances_guard.expect_no_instances();
    CHECK(a.empty());
    CHECK(0 == a.size());
    CHECK(0 == a.capacity());
    CHECK(nullptr == a.data());
  }

  Element::NoNewInstancesGuard instances_guard;
};

class CorrectnessTest : public BaseTest {};

class ExceptionSafetyTest : public BaseTest {};

class PerformanceTest : public BaseTest {};

TEST_CASE_METHOD(CorrectnessTest, "default_ctor") {
  Vector<Element> a;
  expect_empty_storage(a);
}

TEST_CASE_METHOD(ExceptionSafetyTest, "non_throwing_default_ctor") {
  faulty_run([] {
    try {
      Vector<Element> a;
    } catch (...) {
      FaultInjectionDisable dg;
      FAIL("default constructor should not throw");
      throw;
    }
  });
}

TEST_CASE_METHOD(CorrectnessTest, "push_back") {
  static constexpr int N = 5000;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  CHECK(N == a.size());
  CHECK(N <= a.capacity());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(ExceptionSafetyTest, "push_back_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    Vector<Element> a;
    for (int i = 0; i < N; ++i) {
      Element x = 2 * i + 1;
      StrongExceptionSafetyGuard sg_1(a);
      StrongExceptionSafetyGuard sg_2(x);
      a.push_back(x);
    }
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "push_back_xvalue_throw") {
  static constexpr int N = 10;
  faulty_run([] {
    Vector<Element> a;
    for (int i = 0; i < N; ++i) {
      Element x = 2 * i + 1;
      a.push_back(std::move(x));
    }
  });
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_from_self") {
  static constexpr int N = 500;

  Vector<Element> a;
  a.push_back(42);
  for (int i = 1; i < N; ++i) {
    a.push_back(a[0]);
  }

  CHECK(N == a.size());
  CHECK(N <= a.capacity());

  for (int i = 0; i < N; ++i) {
    REQUIRE(42 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_xvalue_from_self_no_reallocation") {
  static constexpr int N = 500;

  Vector<Element> a;
  a.reserve(N);
  a.push_back(42);
  for (int i = 1; i < N; ++i) {
    a.push_back(std::move(a[i - 1]));
  }

  CHECK(N == a.size());
  CHECK(N <= a.capacity());
  for (int i = 0; i < N - 1; ++i) {
    REQUIRE(-1 == a[i]);
  }
  REQUIRE(42 == a[N - 1]);
}

TEST_CASE_METHOD(ExceptionSafetyTest, "push_back_from_self_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    Vector<Element> a;
    a.push_back(42);
    for (int i = 1; i < N; ++i) {
      StrongExceptionSafetyGuard sg(a);
      a.push_back(a[0]);
    }
  });
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_reallocation") {
  static constexpr int N = 500;

  Vector<Element> a;
  a.reserve(N);
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Element x = N;
  Element::reset_counters();
  a.push_back(x);
  REQUIRE(N + 1 == Element::get_copy_counter());
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_reallocation_noexcept") {
  static constexpr int N = 500;

  Vector<ElementWithNonThrowingMove> a;
  a.reserve(N);
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  ElementWithNonThrowingMove x = N;
  Element::reset_counters();
  a.push_back(std::move(x));
  REQUIRE(Element::get_copy_counter() == 0);
  REQUIRE(Element::get_move_counter() <= 501);
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_xvalue_from_self_reallocation") {
  static constexpr int N = 500;

  Vector<Element> a;
  a.push_back(42);
  for (int i = 1; i < N; ++i) {
    a.push_back(std::move(a[i - 1]));
  }

  CHECK(N == a.size());
  CHECK(N <= a.capacity());
  for (int i = 0; i < N - 1; ++i) {
    REQUIRE(-1 == a[i]);
  }
  REQUIRE(42 == a[N - 1]);
}

TEST_CASE_METHOD(CorrectnessTest, "push_back_xvalue_from_self_reallocation_noexcept") {
  static constexpr int N = 500;

  Vector<ElementWithNonThrowingMove> a;
  a.push_back(42);
  for (int i = 1; i < N; ++i) {
    a.push_back(std::move(a[i - 1]));
  }

  CHECK(N == a.size());
  CHECK(N <= a.capacity());
  for (int i = 0; i < N - 1; ++i) {
    REQUIRE(-1 == a[i]);
  }
  REQUIRE(42 == a[N - 1]);
}

TEST_CASE_METHOD(CorrectnessTest, "subscripting") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  const Vector<Element>& ca = a;

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == ca[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "data") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  {
    Element* ptr = a.data();
    for (int i = 0; i < N; ++i) {
      REQUIRE(2 * i + 1 == ptr[i]);
    }
  }

  {
    const Element* cptr = std::as_const(a).data();
    for (int i = 0; i < N; ++i) {
      REQUIRE(2 * i + 1 == cptr[i]);
    }
  }
}

TEST_CASE_METHOD(CorrectnessTest, "front_back") {
  static constexpr int N = 500;
  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  CHECK(1 == a.front());
  CHECK(1 == std::as_const(a).front());

  CHECK(&a[0] == &a.front());
  CHECK(&a[0] == &std::as_const(a).front());

  CHECK(2 * N - 1 == a.back());
  CHECK(2 * N - 1 == std::as_const(a).back());

  CHECK(&a[N - 1] == &a.back());
  CHECK(&a[N - 1] == &std::as_const(a).back());
}

TEST_CASE_METHOD(CorrectnessTest, "reserve") {
  static constexpr int N = 500, M = 100, K = 5000;

  Vector<Element> a;
  a.reserve(N);
  CHECK(0 == a.size());
  CHECK(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  CHECK(M == a.size());
  CHECK(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  a.reserve(K);
  CHECK(M == a.size());
  CHECK(K == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "reserve_superfluous") {
  static constexpr int N = 5000, M = 100, K = 500;

  Vector<Element> a;
  a.reserve(N);
  REQUIRE(0 == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  REQUIRE(M == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  Element* old_data = a.data();

  a.reserve(K);
  CHECK(M == a.size());
  CHECK(N == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "reserve_empty") {
  Vector<Element> a;
  a.reserve(0);
  expect_empty_storage(a);
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reserve_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    a.reserve(N + 1);
  });
}

TEST_CASE_METHOD(CorrectnessTest, "reserve_noexcept") {
  static constexpr int N = 500, M = 100, K = 5000;

  Vector<ElementWithNonThrowingMove> a;
  a.reserve(N);
  CHECK(0 == a.size());
  CHECK(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  CHECK(M == a.size());
  CHECK(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  Element::reset_counters();
  a.reserve(K);

  REQUIRE(Element::get_copy_counter() == 0);
  REQUIRE(Element::get_move_counter() <= 100);

  CHECK(M == a.size());
  CHECK(K == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "shrink_to_fit") {
  static constexpr int N = 500, M = 100;

  Vector<Element> a;
  a.reserve(N);
  REQUIRE(0 == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  REQUIRE(M == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  a.shrink_to_fit();
  CHECK(M == a.size());
  CHECK(M == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "shrink_to_fit_superfluous") {
  static constexpr int N = 500;

  Vector<Element> a;
  a.reserve(N);
  REQUIRE(0 == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  CHECK(N == a.size());

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  a.shrink_to_fit();
  CHECK(N == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());
}

TEST_CASE_METHOD(CorrectnessTest, "shrink_to_fit_empty") {
  Vector<Element> a;
  a.shrink_to_fit();
  expect_empty_storage(a);
}

TEST_CASE_METHOD(ExceptionSafetyTest, "shrink_to_fit_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N * 2);

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    a.shrink_to_fit();
  });
}

TEST_CASE_METHOD(CorrectnessTest, "shrink_to_fit_noexcept") {
  static constexpr int N = 500, M = 100;

  Vector<ElementWithNonThrowingMove> a;
  a.reserve(N);
  REQUIRE(0 == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  REQUIRE(M == a.size());
  REQUIRE(N == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }

  Element::reset_counters();
  a.shrink_to_fit();
  REQUIRE(Element::get_copy_counter() == 0);
  REQUIRE(Element::get_move_counter() <= 100);

  CHECK(M == a.size());
  CHECK(M == a.capacity());

  for (int i = 0; i < M; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "clear") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  REQUIRE(N == a.size());

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  a.clear();
  instances_guard.expect_no_instances();
  CHECK(a.empty());
  CHECK(0 == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());
}

TEST_CASE_METHOD(ExceptionSafetyTest, "non_throwing_clear") {
  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    for (int i = 0; i < 10; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();
    try {
      a.clear();
    } catch (...) {
      FaultInjectionDisable dg_2;
      FAIL("clear() should not throw");
      throw;
    }
  });
}

TEST_CASE_METHOD(CorrectnessTest, "copy_ctor") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Vector<Element> b = a;
  CHECK(a.size() == b.size());
  CHECK(a.size() == b.capacity());
  CHECK(a.data() != b.data());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == b[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "move_ctor") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Element* a_data = a.data();

  Element::reset_counters();
  Vector<Element> b = std::move(a);
  REQUIRE(0 == Element::get_copy_counter());

  CHECK(N == b.size());
  CHECK(N <= b.capacity());
  CHECK(a_data == b.data());
  CHECK(a.data() != b.data());

  CHECK(nullptr == a.data());
  CHECK(0 == a.size());
  CHECK(0 == a.capacity());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == b[i]);
  }
}

TEST_CASE_METHOD(PerformanceTest, "move_ctor") {
  static constexpr int N = 8'000;

  Vector<Vector<int>> a;
  for (int i = 0; i < N; ++i) {
    Vector<int> b;
    for (int j = 0; j < N; ++j) {
      b.push_back(2 * i + 3 * j);
    }
    a.push_back(std::move(b));
  }

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      REQUIRE(2 * i + 3 * j == a[i][j]);
    }
  }
}

TEST_CASE_METHOD(CorrectnessTest, "copy_assignment_operator") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Vector<Element> b;
  b = a;
  CHECK(a.size() == b.size());
  CHECK(a.size() == b.capacity());
  CHECK(a.data() != b.data());

  Vector<Element> c;
  c.push_back(42);
  c = a;
  CHECK(a.size() == c.size());
  CHECK(a.size() == c.capacity());
  CHECK(a.data() != c.data());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
    REQUIRE(2 * i + 1 == b[i]);
    REQUIRE(2 * i + 1 == c[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "move_assignment_operator_to_empty") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Element* a_data = a.data();

  Element::reset_counters();
  Vector<Element> b;
  b = std::move(a);
  REQUIRE(0 == Element::get_copy_counter());

  CHECK(N == b.size());
  CHECK(N <= b.capacity());
  CHECK(a_data == b.data());
  CHECK(a.data() != b.data());

  CHECK(nullptr == a.data());
  CHECK(0 == a.size());
  CHECK(0 == a.capacity());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == b[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "move_assignment_operator_to_non_empty") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Element* a_data = a.data();

  Vector<Element> b;
  b.push_back(42);

  Element::reset_counters();
  b = std::move(a);
  REQUIRE(0 == Element::get_copy_counter());

  CHECK(N == b.size());
  CHECK(N <= b.capacity());
  CHECK(a_data == b.data());
  CHECK(a.data() != b.data());

  CHECK(nullptr == a.data());
  CHECK(0 == a.size());
  CHECK(0 == a.capacity());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == b[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "self_copy_assignment") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  Element::reset_counters();
  a = a;
  REQUIRE(0 == Element::get_copy_counter());
  REQUIRE(0 == Element::get_move_counter());

  CHECK(N == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "self_move_assignment") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  Element::reset_counters();
  a = std::move(a);
  REQUIRE(0 == Element::get_copy_counter());
  REQUIRE(0 == Element::get_move_counter());

  CHECK(static_cast<size_t>(N) == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(PerformanceTest, "move_assignment") {
  static constexpr int N = 8'000;

  Vector<Vector<int>> a;
  for (int i = 0; i < N; ++i) {
    Vector<int> b;
    for (int j = 0; j < N; ++j) {
      b.push_back(2 * i + 3 * j);
    }
    a.push_back({});
    a.back() = std::move(b);
  }

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      REQUIRE(2 * i + 3 * j == a[i][j]);
    }
  }
}

TEST_CASE_METHOD(CorrectnessTest, "empty_storage") {
  Vector<Element> a;
  expect_empty_storage(a);

  Vector<Element> b = a;
  expect_empty_storage(b);

  a = b;
  expect_empty_storage(a);
}

TEST_CASE_METHOD(CorrectnessTest, "pop_back") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  for (int i = N; i > 0; --i) {
    REQUIRE(2 * i - 1 == a.back());
    REQUIRE(static_cast<size_t>(i) == a.size());
    a.pop_back();
  }
  instances_guard.expect_no_instances();
  CHECK(a.empty());
  CHECK(0 == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());
}

TEST_CASE_METHOD(CorrectnessTest, "destroy_order") {
  Vector<OrderedElement> a;

  a.push_back(1);
  a.push_back(2);
  a.push_back(3);
}

TEST_CASE_METHOD(CorrectnessTest, "insert_begin") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    Element x = 2 * i + 1;
    auto it = a.insert(std::as_const(a).begin(), x);
    REQUIRE(a.begin() == it);
    REQUIRE(i + 1uz == a.size());
  }

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a.back());
    a.pop_back();
  }
  REQUIRE(a.empty());
}

TEST_CASE_METHOD(CorrectnessTest, "insert_end") {
  static constexpr int N = 500;

  Vector<Element> a;

  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  REQUIRE(static_cast<size_t>(N) == a.size());

  for (int i = 0; i < N; ++i) {
    Element x = 4 * i + 1;
    auto it = a.insert(a.end(), x);
    REQUIRE(a.end() - 1 == it);
    REQUIRE(N + i + 1uz == a.size());
  }

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
  for (int i = 0; i < N; ++i) {
    REQUIRE(4 * i + 1 == a[N + i]);
  }
}

TEST_CASE_METHOD(PerformanceTest, "insert") {
  static constexpr int N = 8'000;

  Vector<Vector<int>> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(Vector<int>());
    for (int j = 0; j < N; ++j) {
      a.back().push_back(2 * (i + 1) + 3 * j);
    }
  }

  Vector<int> temp;
  for (int i = 0; i < N; ++i) {
    temp.push_back(3 * i);
  }
  auto it = a.insert(a.begin(), temp);
  CHECK(a.begin() == it);

  for (int i = 0; i <= N; ++i) {
    for (int j = 0; j < N; ++j) {
      REQUIRE(2 * i + 3 * j == a[i][j]);
    }
  }
}

TEST_CASE_METHOD(CorrectnessTest, "insert_xvalue_reallocation") {
  static constexpr int N = 500, K = 7;

  Vector<Element> a;
  a.reserve(N);
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  Element x = N;
  Element::reset_counters();
  a.insert(a.begin() + K, std::move(x));

  REQUIRE(Element::get_copy_counter() <= 500);
}

TEST_CASE_METHOD(CorrectnessTest, "insert_xvalue_reallocation_noexcept") {
  static constexpr int N = 500, K = 0;

  Vector<ElementWithNonThrowingMove> a;
  a.reserve(N);
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  ElementWithNonThrowingMove x = N;
  Element::reset_counters();
  a.insert(a.begin() + K, std::move(x));

  REQUIRE(Element::get_copy_counter() == 0);
}

TEST_CASE_METHOD(CorrectnessTest, "erase") {
  static constexpr int N = 500;

  for (int i = 0; i < N; ++i) {
    Vector<Element> a;
    for (int j = 0; j < N; ++j) {
      a.push_back(2 * j + 1);
    }

    size_t old_capacity = a.capacity();
    Element* old_data = a.data();

    auto it = a.erase(std::as_const(a).begin() + i);
    REQUIRE(a.begin() + i == it);
    REQUIRE(N - 1 == a.size());
    REQUIRE(old_capacity == a.capacity());
    REQUIRE(old_data == a.data());

    for (int j = 0; j < i; ++j) {
      REQUIRE(2 * j + 1 == a[j]);
    }
    for (int j = i; j < N - 1; ++j) {
      REQUIRE(2 * (j + 1) + 1 == a[j]);
    }
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_begin") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N * 2; ++i) {
    a.push_back(2 * i + 1);
  }

  for (int i = 0; i < N; ++i) {
    auto it = a.erase(a.begin());
    REQUIRE(a.begin() == it);
  }

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * (i + N) + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_end") {
  static constexpr int N = 500;

  Vector<Element> a;
  for (int i = 0; i < N * 2; ++i) {
    a.push_back(2 * i + 1);
  }

  for (int i = 0; i < N; ++i) {
    auto it = a.erase(a.end() - 1);
    REQUIRE(a.end() == it);
  }

  for (int i = 0; i < N; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_range_begin") {
  static constexpr int N = 500, K = 100;

  Vector<Element> a;
  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  auto it = a.erase(std::as_const(a).begin(), std::as_const(a).begin() + K);
  CHECK(a.begin() == it);
  CHECK(N - K == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < N - K; ++i) {
    REQUIRE(2 * (i + K) + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_range_middle") {
  static constexpr int N = 500, K = 100;

  Vector<Element> a;

  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  auto it = a.erase(a.begin() + K, a.end() - K);
  CHECK(a.begin() + K == it);
  CHECK(K * 2 == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < K; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
  for (int i = 0; i < K; ++i) {
    REQUIRE(2 * (i + N - K) + 1 == a[i + K]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_range_end") {
  static constexpr int N = 500, K = 100;

  Vector<Element> a;

  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  auto it = a.erase(a.end() - K, a.end());
  CHECK(a.end() == it);
  CHECK(N - K == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());

  for (int i = 0; i < N - K; ++i) {
    REQUIRE(2 * i + 1 == a[i]);
  }
}

TEST_CASE_METHOD(CorrectnessTest, "erase_range_all") {
  static constexpr int N = 500;

  Vector<Element> a;

  for (int i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  Element* old_data = a.data();

  auto it = a.erase(a.begin(), a.end());
  CHECK(a.end() == it);

  instances_guard.expect_no_instances();
  CHECK(a.empty());
  CHECK(0 == a.size());
  CHECK(old_capacity == a.capacity());
  CHECK(old_data == a.data());
}

TEST_CASE_METHOD(PerformanceTest, "erase") {
  static constexpr int N = 8'000, M = 50'000, K = 100;

  Vector<int> a;
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < M; ++j) {
      a.push_back(j);
    }
    auto it = a.erase(a.begin() + K, a.end() - K);
    REQUIRE(a.begin() + K == it);
    REQUIRE(K * 2 == a.size());
    a.clear();
  }
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reallocation_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    Element x = 42;

    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    a.push_back(x);
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reallocation_push_back_from_self_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    a.push_back(std::move(a[0]));
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reallocation_throw_push_back_from_self_noexcept_move") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<ElementWithNonThrowingMove> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    FaultInjectionMoveThrowDisable mdg;
    StrongExceptionSafetyGuard sg(a);
    a.push_back(std::move(a[0]));
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reallocation_insert_throw") {
  static constexpr int N = 500;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    Element x = 42;
    dg.reset();

    [[maybe_unused]] auto it = a.insert(std::as_const(a).begin(), std::move(x));
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "reallocation_throw_insert_noexcept") {
  static constexpr int N = 500;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<ElementWithNonThrowingMove> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }

    dg.reset();

    FaultInjectionMoveThrowDisable mdg;
    StrongExceptionSafetyGuard sg(a);
    ElementWithNonThrowingMove x = 42;
    [[maybe_unused]] auto it = a.insert(std::as_const(a).begin(), std::move(x));
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "copy_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    [[maybe_unused]] Vector<Element> b(a);
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "move_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);
    REQUIRE(N == a.capacity());

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    [[maybe_unused]] Vector<Element> b(std::move(a));
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "copy_assign_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }

    Vector<Element> b;
    b.push_back(0);
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    b = std::as_const(a);
  });
}

TEST_CASE_METHOD(ExceptionSafetyTest, "move_assign_throw") {
  static constexpr int N = 10;

  faulty_run([] {
    FaultInjectionDisable dg;
    Vector<Element> a;
    a.reserve(N);

    for (int i = 0; i < N; ++i) {
      a.push_back(2 * i + 1);
    }

    Vector<Element> b;
    b.push_back(0);
    dg.reset();

    StrongExceptionSafetyGuard sg(a);
    b = std::move(a);
  });
}

TEST_CASE_METHOD(CorrectnessTest, "member_aliases") {
  CHECK((std::is_same<Element, Vector<Element>::ValueType>::value));
  CHECK((std::is_same<Element&, Vector<Element>::Reference>::value));
  CHECK((std::is_same<const Element&, Vector<Element>::ConstReference>::value));
  CHECK((std::is_same<Element*, Vector<Element>::Pointer>::value));
  CHECK((std::is_same<const Element*, Vector<Element>::ConstPointer>::value));
  CHECK((std::is_same<Element*, Vector<Element>::Iterator>::value));
  CHECK((std::is_same<const Element*, Vector<Element>::ConstIterator>::value));
}

} // namespace test
} // namespace ct
