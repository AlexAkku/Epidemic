#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace epidemic::gameplay::presentation {
struct ClockFrame {
  double ui_delta = 0, simulation_step = 0, interpolation = 0;
  double ui_time = 0, simulation_time = 0, media_position = 0;
  double dropped_simulation_seconds = 0;
  unsigned simulation_steps = 0;
};

// The caller executes simulation_steps fixed updates, then draws once. UI uses
// unscaled wall time; media_position comes from the actual audio sample cursor.
// This policy never seeks/pauses audio itself and cannot replace an audio
// clock.
class FrameClocks {
  double step_, accumulator_ = 0, ui_time_ = 0;
  std::uint64_t ticks_ = 0;
  unsigned max_steps_;

public:
  explicit FrameClocks(double simulation_step = 1.0 / 120,
                       unsigned max_steps = 8)
      : step_(simulation_step), max_steps_(max_steps) {
    if (!std::isfinite(step_) || step_ <= 0 || !max_steps_ ||
        !std::isfinite(step_ * max_steps_))
      throw std::invalid_argument("Invalid fixed-step policy");
  }
  void ClearDebt() { accumulator_ = 0; }
  void Reset() {
    accumulator_ = 0;
    ui_time_ = 0;
    ticks_ = 0;
  }
  ClockFrame Advance(double wall_seconds, double time_scale = 1,
                     bool paused = false, double media_position = 0) {
    if (!std::isfinite(wall_seconds) || wall_seconds < 0 ||
        !std::isfinite(time_scale) || time_scale < 0 ||
        !std::isfinite(media_position) || media_position < 0)
      throw std::invalid_argument("Invalid frame clock input");
    if (!std::isfinite(ui_time_ + wall_seconds))
      throw std::invalid_argument("UI time overflow");
    ClockFrame frame;
    frame.ui_delta = wall_seconds;
    frame.ui_time = ui_time_ += wall_seconds;
    frame.media_position = media_position;
    frame.simulation_step = step_;
    if (paused || time_scale == 0) {
      ClearDebt(); // Unpausing cannot replay stale input/time debt.
    } else {
      const double scaled = wall_seconds * time_scale;
      if (!std::isfinite(scaled))
        throw std::invalid_argument("Simulation delta overflow");
      const double capacity = step_ * max_steps_;
      const double available = accumulator_ + scaled;
      if (!std::isfinite(available))
        throw std::invalid_argument("Simulation debt overflow");
      accumulator_ = std::min(available, capacity);
      frame.dropped_simulation_seconds = std::max(0.0, available - capacity);
      const double steps = std::floor(accumulator_ / step_);
      frame.simulation_steps =
          steps >= max_steps_ ? max_steps_ : static_cast<unsigned>(steps);
      accumulator_ -= frame.simulation_steps * step_;
      ticks_ += frame.simulation_steps;
    }
    frame.simulation_time = static_cast<double>(ticks_) * step_;
    frame.interpolation = std::clamp(accumulator_ / step_, 0.0, 1.0);
    return frame;
  }
};
} // namespace epidemic::gameplay::presentation
