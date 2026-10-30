#pragma once
// Audit-only probes; implementations live in the existing uninstrumented
// runtime.
#include <cstdint>
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include <array>
#include <string>
namespace epidemic::auditor::shadow {
using Identity = std::array<char, 129>;
struct Context {
  Identity trace{}, span{}, parent_span{}, cause{}, action{}, scenario{};
};
class ContextScope {
  Context previous_;
  std::string previous_action_;

public:
  explicit ContextScope(const Context &next) noexcept;
  ~ContextScope() noexcept;
  ContextScope(const ContextScope &) = delete;
  ContextScope &operator=(const ContextScope &) = delete;
};
Identity identity() noexcept;
Context context() noexcept;
Context child_context() noexcept;
Identity emit_causal(const char *, const char *) noexcept;
Identity emit_causal(const char *, std::uint64_t) noexcept;
void causal(const char *, const char *) noexcept;
void causal(const char *, std::uint64_t) noexcept;
void flow(const char *, const char *) noexcept;
void flow(const char *, std::uint64_t) noexcept;
void action(const char *, const char *, const char * = nullptr) noexcept;
void arm_next_input(const char *, const char * = nullptr) noexcept;
void input(const char *) noexcept;
void frame(std::uint64_t) noexcept;
void scenario(const char *, const char *) noexcept;
void entity_change(std::uint64_t, std::uint64_t, unsigned, unsigned,
                   unsigned) noexcept;
void io(const char *, const char *, std::uint64_t, std::uint64_t) noexcept;
void allocation(const char *, std::uint64_t, std::uint64_t,
                const char * = nullptr) noexcept;
bool fail_allocation(const char *) noexcept;
#ifdef _WIN32
void delay_io(const wchar_t *) noexcept;
#endif
void exception(const char *, std::uint64_t, std::uint64_t) noexcept;
} // namespace epidemic::auditor::shadow

#else
namespace epidemic::auditor::shadow {
inline void causal(const char *, const char *) noexcept {}
inline void causal(const char *, std::uint64_t) noexcept {}
inline void action(const char *, const char *) noexcept {}
inline void scenario(const char *, const char *) noexcept {}
inline void io(const char *, const char *, std::uint64_t,
               std::uint64_t) noexcept {}
inline void allocation(const char *, std::uint64_t, std::uint64_t,
                       const char * = nullptr) noexcept {}
inline void exception(const char *, std::uint64_t, std::uint64_t) noexcept {}
} // namespace epidemic::auditor::shadow
#endif
