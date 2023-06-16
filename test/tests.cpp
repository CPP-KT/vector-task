#include "element.h"
#include "vector.h"

#include <gtest/gtest.h>

#include <string>

template class vector<int>;
template class vector<element>;
template class vector<std::string>;

namespace {

class base_test : public ::testing::Test {
protected:
  void SetUp() override {
    element::instances().clear();
    element::reset_copies();
  }

  void TearDown() override {
    element::expect_no_instances();
  }
};

class correctness_test : public base_test {};

class performance_test : public base_test {};

void expect_empty_storage(const vector<element>& a) {
  element::expect_no_instances();
  EXPECT_TRUE(a.empty());
  EXPECT_EQ(0, a.size());
  EXPECT_EQ(0, a.capacity());
  EXPECT_EQ(nullptr, a.data());
}

} // namespace

#define EXPECT_STRONG_EXCEPTION_SAFETY(a, action, failing_position)                                                    \
  {                                                                                                                    \
    auto old_a = a;                                                                                                    \
    auto old_capacity = a.capacity();                                                                                  \
    auto old_data = a.data();                                                                                          \
    element::set_throw_countdown(failing_position);                                                                    \
    EXPECT_THROW(action, std::runtime_error);                                                                          \
    EXPECT_EQ(old_a.size(), a.size());                                                                                 \
    EXPECT_EQ(old_capacity, a.capacity());                                                                             \
    EXPECT_EQ(old_data, a.data());                                                                                     \
    for (size_t i = 0; i < old_a.size(); ++i) {                                                                        \
      ASSERT_EQ(old_a[i], a[i]);                                                                                       \
    }                                                                                                                  \
  }

TEST_F(correctness_test, default_ctor) {
  vector<element> a;
  expect_empty_storage(a);
}

TEST_F(correctness_test, push_back) {
  constexpr size_t N = 5000;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_EQ(N, a.size());
  EXPECT_LE(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, push_back_from_self) {
  constexpr size_t N = 500;

  vector<element> a;
  a.push_back(42);
  for (size_t i = 1; i < N; ++i) {
    a.push_back(a[0]);
  }

  EXPECT_EQ(N, a.size());
  EXPECT_LE(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(42, a[i]);
  }
}

TEST_F(correctness_test, push_back_reallocation) {
  constexpr size_t N = 500;

  vector<element> a;
  a.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  element::reset_copies();
  a.push_back(N);
  element::expect_copies(N + 1);
}

TEST_F(correctness_test, subscripting) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }

  const vector<element>& ca = a;

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, ca[i]);
  }
}

TEST_F(correctness_test, data) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  {
    element* ptr = a.data();
    for (size_t i = 0; i < N; ++i) {
      ASSERT_EQ(2 * i + 1, ptr[i]);
    }
  }

  {
    const element* cptr = std::as_const(a).data();
    for (size_t i = 0; i < N; ++i) {
      ASSERT_EQ(2 * i + 1, cptr[i]);
    }
  }
}

TEST_F(correctness_test, front_back) {
  constexpr size_t N = 500;
  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_EQ(1, a.front());
  EXPECT_EQ(1, std::as_const(a).front());

  EXPECT_EQ(&a[0], &a.front());
  EXPECT_EQ(&a[0], &std::as_const(a).front());

  EXPECT_EQ(2 * N - 1, a.back());
  EXPECT_EQ(2 * N - 1, std::as_const(a).back());

  EXPECT_EQ(&a[N - 1], &a.back());
  EXPECT_EQ(&a[N - 1], &std::as_const(a).back());
}

TEST_F(correctness_test, reserve) {
  constexpr size_t N = 500, M = 100, K = 5000;

  vector<element> a;
  a.reserve(N);
  EXPECT_EQ(0, a.size());
  EXPECT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  EXPECT_EQ(M, a.size());
  EXPECT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }

  a.reserve(K);
  EXPECT_EQ(M, a.size());
  EXPECT_EQ(K, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, reserve_superfluous) {
  constexpr size_t N = 5000, M = 100, K = 500;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(0, a.size());
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  ASSERT_EQ(M, a.size());
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }

  element* old_data = a.data();

  a.reserve(K);
  EXPECT_EQ(M, a.size());
  EXPECT_EQ(N, a.capacity());
  EXPECT_EQ(old_data, a.data());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, reserve_empty) {
  vector<element> a;
  a.reserve(0);
  expect_empty_storage(a);
}

TEST_F(correctness_test, reserve_throw) {
  constexpr size_t N = 10, K = 7;

  vector<element> a;
  a.reserve(N);

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_STRONG_EXCEPTION_SAFETY(a, a.reserve(N + 1), K);
}

TEST_F(correctness_test, shrink_to_fit) {
  constexpr size_t N = 500, M = 100;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(0, a.size());
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    a.push_back(2 * i + 1);
  }
  ASSERT_EQ(M, a.size());
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }

  a.shrink_to_fit();
  EXPECT_EQ(M, a.size());
  EXPECT_EQ(M, a.capacity());

  for (size_t i = 0; i < M; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, shrink_to_fit_superfluous) {
  constexpr size_t N = 500;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(0, a.size());
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  ASSERT_EQ(N, a.size());

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  a.shrink_to_fit();
  EXPECT_EQ(N, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());
}

TEST_F(correctness_test, shrink_to_fit_empty) {
  vector<element> a;
  a.shrink_to_fit();
  expect_empty_storage(a);
}

TEST_F(correctness_test, shrink_to_fit_throw) {
  constexpr size_t N = 10, K = 7;

  vector<element> a;
  a.reserve(N * 2);

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_STRONG_EXCEPTION_SAFETY(a, a.shrink_to_fit(), K);
}

TEST_F(correctness_test, clear) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  ASSERT_EQ(N, a.size());

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  a.clear();
  element::expect_no_instances();
  EXPECT_TRUE(a.empty());
  EXPECT_EQ(0, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());
}

TEST_F(correctness_test, copy_ctor) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  vector<element> b = a;
  EXPECT_EQ(a.size(), b.size());
  EXPECT_EQ(a.size(), b.capacity());
  EXPECT_NE(a.data(), b.data());

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, b[i]);
  }
}

TEST_F(correctness_test, assignment_operator) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  vector<element> b;
  b = a;
  EXPECT_EQ(a.size(), b.size());
  EXPECT_EQ(a.size(), b.capacity());
  EXPECT_NE(a.data(), b.data());

  vector<element> c;
  c.push_back(42);
  c = a;
  EXPECT_EQ(a.size(), c.size());
  EXPECT_EQ(a.size(), c.capacity());
  EXPECT_NE(a.data(), c.data());

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
    ASSERT_EQ(2 * i + 1, b[i]);
    ASSERT_EQ(2 * i + 1, c[i]);
  }
}

TEST_F(correctness_test, self_assignment) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  a = a;
  EXPECT_EQ(N, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, empty_storage) {
  vector<element> a;
  expect_empty_storage(a);

  vector<element> b = a;
  expect_empty_storage(b);

  a = b;
  expect_empty_storage(a);
}

TEST_F(correctness_test, pop_back) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  for (size_t i = N; i > 0; --i) {
    ASSERT_EQ(2 * i - 1, a.back());
    ASSERT_EQ(i, a.size());
    a.pop_back();
  }
  element::expect_no_instances();
  EXPECT_TRUE(a.empty());
  EXPECT_EQ(0, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());
}

TEST_F(correctness_test, insert_begin) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    auto it = a.insert(as_const(a).begin(), 2 * i + 1);
    ASSERT_EQ(a.begin(), it);
    ASSERT_EQ(i + 1, a.size());
  }

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a.back());
    a.pop_back();
  }
  ASSERT_TRUE(a.empty());
}

TEST_F(correctness_test, insert_end) {
  constexpr size_t N = 500;

  vector<element> a;

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }
  ASSERT_EQ(N, a.size());

  for (size_t i = 0; i < N; ++i) {
    auto it = a.insert(a.end(), 4 * i + 1);
    ASSERT_EQ(a.end() - 1, it);
    ASSERT_EQ(N + i + 1, a.size());
  }

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(4 * i + 1, a[N + i]);
  }
}

TEST_F(performance_test, insert) {
  constexpr size_t N = 10000;

  vector<vector<int>> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(vector<int>());
    for (size_t j = 0; j < N; ++j) {
      a.back().push_back(2 * (i + 1) + 3 * j);
    }
  }

  vector<int> temp;
  for (size_t i = 0; i < N; ++i) {
    temp.push_back(3 * i);
  }
  auto it = a.insert(a.begin(), temp);
  EXPECT_EQ(a.begin(), it);

  for (size_t i = 0; i <= N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      ASSERT_EQ(2 * i + 3 * j, a[i][j]);
    }
  }
}

TEST_F(correctness_test, erase) {
  constexpr size_t N = 500;

  for (size_t i = 0; i < N; ++i) {
    vector<element> a;
    for (size_t j = 0; j < N; ++j) {
      a.push_back(2 * j + 1);
    }

    size_t old_capacity = a.capacity();
    element* old_data = a.data();

    auto it = a.erase(as_const(a).begin() + i);
    ASSERT_EQ(a.begin() + i, it);
    ASSERT_EQ(N - 1, a.size());
    ASSERT_EQ(old_capacity, a.capacity());
    ASSERT_EQ(old_data, a.data());

    for (size_t j = 0; j < i; ++j) {
      ASSERT_EQ(2 * j + 1, a[j]);
    }
    for (size_t j = i; j < N - 1; ++j) {
      ASSERT_EQ(2 * (j + 1) + 1, a[j]);
    }
  }
}

TEST_F(correctness_test, erase_begin) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N * 2; ++i) {
    a.push_back(2 * i + 1);
  }

  for (size_t i = 0; i < N; ++i) {
    auto it = a.erase(a.begin());
    ASSERT_EQ(a.begin(), it);
  }

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * (i + N) + 1, a[i]);
  }
}

TEST_F(correctness_test, erase_end) {
  constexpr size_t N = 500;

  vector<element> a;
  for (size_t i = 0; i < N * 2; ++i) {
    a.push_back(2 * i + 1);
  }

  for (size_t i = 0; i < N; ++i) {
    auto it = a.erase(a.end() - 1);
    ASSERT_EQ(a.end(), it);
  }

  for (size_t i = 0; i < N; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, erase_range_begin) {
  constexpr size_t N = 500, K = 100;

  vector<element> a;
  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  auto it = a.erase(as_const(a).begin(), as_const(a).begin() + K);
  EXPECT_EQ(a.begin(), it);
  EXPECT_EQ(N - K, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());

  for (size_t i = 0; i < N - K; ++i) {
    ASSERT_EQ(2 * (i + K) + 1, a[i]);
  }
}

TEST_F(correctness_test, erase_range_middle) {
  constexpr size_t N = 500, K = 100;

  vector<element> a;

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  auto it = a.erase(a.begin() + K, a.end() - K);
  EXPECT_EQ(a.begin() + K, it);
  EXPECT_EQ(K * 2, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());

  for (size_t i = 0; i < K; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
  for (size_t i = 0; i < K; ++i) {
    ASSERT_EQ(2 * (i + N - K) + 1, a[i + K]);
  }
}

TEST_F(correctness_test, erase_range_end) {
  constexpr size_t N = 500, K = 100;

  vector<element> a;

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  auto it = a.erase(a.end() - K, a.end());
  EXPECT_EQ(a.end(), it);
  EXPECT_EQ(N - K, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());

  for (size_t i = 0; i < N - K; ++i) {
    ASSERT_EQ(2 * i + 1, a[i]);
  }
}

TEST_F(correctness_test, erase_range_all) {
  constexpr size_t N = 500;

  vector<element> a;

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  size_t old_capacity = a.capacity();
  element* old_data = a.data();

  auto it = a.erase(a.begin(), a.end());
  EXPECT_EQ(a.end(), it);

  element::expect_no_instances();
  EXPECT_TRUE(a.empty());
  EXPECT_EQ(0, a.size());
  EXPECT_EQ(old_capacity, a.capacity());
  EXPECT_EQ(old_data, a.data());
}

TEST_F(performance_test, erase) {
  constexpr size_t N = 10'000, M = 50'000, K = 100;

  vector<int> a;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      a.push_back(j);
    }
    auto it = a.erase(a.begin() + K, a.end() - K);
    ASSERT_EQ(a.begin() + K, it);
    ASSERT_EQ(K * 2, a.size());
    a.clear();
  }
}

TEST_F(correctness_test, reallocation_throw) {
  constexpr size_t N = 10;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_STRONG_EXCEPTION_SAFETY(a, a.push_back(42), N - 1);
}

TEST_F(correctness_test, last_copy_at_reallocation_throw) {
  constexpr size_t N = 10;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_STRONG_EXCEPTION_SAFETY(a, a.push_back(42), N + 1);
}

// This test actually checks memory leak in pair with @valgrind
TEST_F(correctness_test, copy_throw) {
  constexpr size_t N = 10, K = 7;

  vector<element> a;
  a.reserve(N);
  ASSERT_EQ(N, a.capacity());

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  EXPECT_STRONG_EXCEPTION_SAFETY(
      a, { vector<element> b(a); }, K);
}

TEST_F(correctness_test, assign_throw) {
  constexpr size_t N = 10, K = 7;

  vector<element> a;
  a.reserve(N);

  for (size_t i = 0; i < N; ++i) {
    a.push_back(2 * i + 1);
  }

  vector<element> b;
  b.push_back(0);

  EXPECT_STRONG_EXCEPTION_SAFETY(
      a, { b = std::as_const(a); }, K);
}

TEST_F(correctness_test, member_aliases) {
  EXPECT_TRUE((std::is_same<element, vector<element>::value_type>::value));
  EXPECT_TRUE((std::is_same<element&, vector<element>::reference>::value));
  EXPECT_TRUE((std::is_same<const element&, vector<element>::const_reference>::value));
  EXPECT_TRUE((std::is_same<element*, vector<element>::pointer>::value));
  EXPECT_TRUE((std::is_same<const element*, vector<element>::const_pointer>::value));
  EXPECT_TRUE((std::is_same<element*, vector<element>::iterator>::value));
  EXPECT_TRUE((std::is_same<const element*, vector<element>::const_iterator>::value));
}
