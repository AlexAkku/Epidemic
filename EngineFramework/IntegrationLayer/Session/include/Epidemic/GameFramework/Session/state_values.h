#pragma once
#include <cstdint>
#include <memory>

namespace epidemic::gameplay::session {
// Value-shaped command/query handles. Authoritative state stays in the existing
// Framework owners; these handles do not contain a second health/account/item
// value.
class HealthValue {
  struct State;
  std::shared_ptr<State> state_;
  bool maximum_ = false;
  HealthValue(std::shared_ptr<State> state, bool maximum);

public:
  explicit HealthValue(double initial = 100);
  HealthValue(const HealthValue &) = default;
  HealthValue(HealthValue &&) noexcept = default;
  HealthValue &operator=(HealthValue &&) noexcept = default;
  HealthValue &operator=(const HealthValue &other);
  HealthValue &operator=(double value);
  HealthValue &operator+=(double delta);
  HealthValue &operator-=(double delta);
  HealthValue &operator*=(double scale);
  operator double() const;
  HealthValue Maximum() const;
  std::uint64_t EntityId() const;
  std::uint64_t LiveEntityCount() const;
};
class CoinValue {
  struct State;
  std::shared_ptr<State> state_;

public:
  CoinValue();
  CoinValue(const CoinValue &) = default;
  CoinValue(CoinValue &&) noexcept = default;
  CoinValue &operator=(CoinValue &&) noexcept = default;
  CoinValue &operator=(const CoinValue &other);
  CoinValue &operator=(int value);
  CoinValue &operator+=(int delta);
  CoinValue &operator-=(int delta);
  operator int() const;
};
class ItemValue {
  struct State;
  std::shared_ptr<State> state_;

public:
  ItemValue(int definition = -1);
  ItemValue(const ItemValue &) = default;
  ItemValue(ItemValue &&) noexcept = default;
  ItemValue &operator=(ItemValue &&) noexcept = default;
  ItemValue &operator=(const ItemValue &other);
  ItemValue &operator=(int definition);
  operator int() const;
};
class ProgressValue {
  struct State;
  std::shared_ptr<State> state_;

public:
  ProgressValue(double initial = 0);
  ProgressValue(const ProgressValue &) = default;
  ProgressValue(ProgressValue &&) noexcept = default;
  ProgressValue &operator=(ProgressValue &&) noexcept = default;
  ProgressValue &operator=(const ProgressValue &other);
  ProgressValue &operator=(double value);
  ProgressValue &operator+=(double delta);
  ProgressValue &operator-=(double delta);
  operator double() const;
};
class RankValue {
  ProgressValue value_;

public:
  RankValue(int initial = 0) : value_(initial) {}
  RankValue &operator=(int value) {
    value_ = value;
    return *this;
  }
  RankValue &operator++() {
    value_ += 1;
    return *this;
  }
  int operator++(int) {
    const int old = *this;
    ++*this;
    return old;
  }
  operator int() const { return static_cast<int>(static_cast<double>(value_)); }
};
} // namespace epidemic::gameplay::session
