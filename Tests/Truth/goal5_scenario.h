#pragma once
#include "goal5_digest.h"
#include <algorithm>
#include <optional>

namespace goal5_test_utility
{
enum class ScenarioStepKind : std::uint8_t
{
    SubmitCommand = 1, AdvanceGameTime, Tick, CompleteExternalOperation, ApplyBudget,
    StreamOut, StreamIn, SaveReload, Reconcile, Materialize
};
enum class ScenarioClockPolicy : std::uint8_t { LogicalExplicit = 1 };
enum class ScenarioSeedPolicy : std::uint8_t { ExplicitOwnedStreams = 1 };
enum class ScenarioFloatPolicy : std::uint8_t { CanonicalQuantized = 1 };
enum class CompletionPolicy : std::uint8_t { ExplicitOrderedInput = 1 };

struct ScenarioStep
{
    ScenarioStepKind kind = ScenarioStepKind::Tick;
    std::uint64_t logical_order = 0;
    std::uint64_t subject_high = 0, subject_low = 0;
    std::int64_t value = 0;
    std::string operation_id;
    CompletionPolicy completion_policy = CompletionPolicy::ExplicitOrderedInput;
};
struct ScenarioDescriptor
{
    std::string scenario_id, config_id;
    std::vector<std::uint64_t> seed_set;
    std::string initial_state_id;
    std::vector<ScenarioStep> steps;
    std::string expected_contract;
    ScenarioClockPolicy clock_policy = ScenarioClockPolicy::LogicalExplicit;
    ScenarioSeedPolicy seed_policy = ScenarioSeedPolicy::ExplicitOwnedStreams;
    ScenarioFloatPolicy floating_policy = ScenarioFloatPolicy::CanonicalQuantized;
};

inline void ValidateScenario(const ScenarioDescriptor& descriptor)
{
    if (descriptor.scenario_id.empty() || descriptor.config_id.empty() || descriptor.seed_set.empty() ||
        descriptor.initial_state_id.empty() || descriptor.steps.empty() || descriptor.expected_contract.empty())
        throw std::runtime_error("incomplete scenario reproduction descriptor");
    if (descriptor.clock_policy != ScenarioClockPolicy::LogicalExplicit ||
        descriptor.seed_policy != ScenarioSeedPolicy::ExplicitOwnedStreams ||
        descriptor.floating_policy != ScenarioFloatPolicy::CanonicalQuantized)
        throw std::runtime_error("ambient clock/random/raw floating policy forbidden");
    std::uint64_t previous = 0;
    for (const auto& step : descriptor.steps)
    {
        const auto kind = static_cast<std::uint8_t>(step.kind);
        if (kind < 1 || kind > 10 || step.logical_order <= previous || step.logical_order > std::numeric_limits<std::uint64_t>::max() / 1024)
            throw std::runtime_error("unknown step, unordered input or missing stable tie breaker");
        if (step.completion_policy != CompletionPolicy::ExplicitOrderedInput ||
            (step.kind == ScenarioStepKind::CompleteExternalOperation && step.operation_id.empty()))
            throw std::runtime_error("async completion must be an explicitly ordered stable input");
        if ((step.kind == ScenarioStepKind::AdvanceGameTime || step.kind == ScenarioStepKind::ApplyBudget) && step.value < 0)
            throw std::runtime_error("negative logical time/work budget");
        previous = step.logical_order;
    }
}

inline std::vector<std::byte> EncodeScenario(ScenarioDescriptor descriptor)
{
    ValidateScenario(descriptor);
    epidemic::gameplay::savegame::detail::SnapshotWriter writer;
    std::uint32_t version = 1;
    epidemic::gameplay::savegame::detail::TransferSnapshot(writer, version);
    epidemic::gameplay::savegame::detail::TransferSnapshot(writer, descriptor);
    return std::move(writer.bytes);
}
inline ScenarioDescriptor DecodeScenario(std::span<const std::byte> bytes)
{
    epidemic::gameplay::savegame::detail::SnapshotReader reader(bytes);
    std::uint32_t version = 0;
    epidemic::gameplay::savegame::detail::TransferSnapshot(reader, version);
    if (version != 1) throw std::runtime_error("unsupported scenario descriptor version");
    ScenarioDescriptor descriptor;
    epidemic::gameplay::savegame::detail::TransferSnapshot(reader, descriptor);
    if (reader.Remaining() != 0) throw std::runtime_error("scenario descriptor trailing data");
    ValidateScenario(descriptor);
    return descriptor;
}

struct SemanticEvent
{
    std::string event_type;
    std::uint64_t subject_high = 0, subject_low = 0;
    std::uint64_t object_high = 0, object_low = 0;
    std::string semantic_payload;
    std::uint64_t logical_order = 0;
    bool operator==(const SemanticEvent&) const = default;
};
class SemanticTrace
{
  public:
    void Commit(SemanticEvent event)
    {
        if (event.event_type.empty() || event.semantic_payload.empty() || event.logical_order == 0 ||
            (!events_.empty() && event.logical_order <= events_.back().logical_order))
            throw std::runtime_error("missing semantic commit or duplicate/unordered event key");
        events_.push_back(std::move(event));
    }
    const std::vector<SemanticEvent>& Events() const noexcept { return events_; }
    std::string Digest() const
    {
        auto snapshot = events_;
        CanonicalSemanticWriter writer;
        epidemic::gameplay::savegame::detail::TransferSnapshot(writer, snapshot);
        return StableDigest(writer.bytes);
    }
  private:
    std::vector<SemanticEvent> events_;
};
struct ScenarioFailure
{
    std::string scenario_id;
    std::vector<std::uint64_t> seed_set;
    std::size_t step_index = 0;
    std::string expected_digest, actual_digest, first_differing_trace_event, message;
};
struct ScenarioResult
{
    std::string semantic_digest, event_trace_digest;
    std::vector<SemanticEvent> normalized_trace;
    std::size_t step_count = 0;
    std::optional<ScenarioFailure> failure;
};

template <typename Port> ScenarioResult RunScenario(const ScenarioDescriptor& descriptor, Port& port,
                                                    SemanticTrace& trace, std::size_t begin = 0,
                                                    std::size_t end = std::numeric_limits<std::size_t>::max())
{
    ScenarioResult result;
    std::size_t index = begin;
    try
    {
        ValidateScenario(descriptor);
        end = std::min(end, descriptor.steps.size());
        if (begin > end) throw std::runtime_error("invalid scenario segment");
        for (; index != end; ++index) { port.Apply(descriptor.steps[index], trace); ++result.step_count; }
        result.semantic_digest = port.Digest();
        result.event_trace_digest = trace.Digest();
        result.normalized_trace = trace.Events();
    }
    catch (const std::exception& error)
    {
        auto actual = std::string("<unavailable after execution failure>");
        try { actual = port.Digest(); } catch (...) { /* Report unavailability explicitly. */ }
        result.failure = ScenarioFailure{descriptor.scenario_id, descriptor.seed_set, index,
            "<reference digest not supplied>", std::move(actual), "<execution failed before comparison>", error.what()};
    }
    catch (...)
    {
        result.failure = ScenarioFailure{descriptor.scenario_id, descriptor.seed_set, index,
            "<reference digest not supplied>", "<unavailable after provider exception>",
            "<execution failed before comparison>", "scenario provider threw a non-standard exception"};
    }
    return result;
}

inline std::optional<ScenarioFailure> CompareScenario(const ScenarioDescriptor& descriptor,
                                                     const ScenarioResult& expected, const ScenarioResult& actual)
{
    if (expected.failure) return expected.failure;
    if (actual.failure) return actual.failure;
    if (expected.semantic_digest == actual.semantic_digest && expected.event_trace_digest == actual.event_trace_digest &&
        expected.normalized_trace == actual.normalized_trace && expected.step_count == actual.step_count) return std::nullopt;
    std::size_t index = 0;
    while (index < expected.normalized_trace.size() && index < actual.normalized_trace.size() &&
           expected.normalized_trace[index] == actual.normalized_trace[index]) ++index;
    auto first = std::string("<state-only divergence>");
    std::size_t step = std::min(expected.step_count, actual.step_count);
    if (index < expected.normalized_trace.size() || index < actual.normalized_trace.size())
    {
        const auto describe = [](const SemanticEvent& event) {
            return event.event_type + " subject=" + std::to_string(event.subject_high) + ":" + std::to_string(event.subject_low) +
                " object=" + std::to_string(event.object_high) + ":" + std::to_string(event.object_low) +
                " order=" + std::to_string(event.logical_order) + " payload=" + event.semantic_payload;
        };
        const auto expected_event = index < expected.normalized_trace.size() ? describe(expected.normalized_trace[index]) : "<end>";
        const auto actual_event = index < actual.normalized_trace.size() ? describe(actual.normalized_trace[index]) : "<end>";
        first = "event " + std::to_string(index) + ": expected=" + expected_event + ", actual=" + actual_event;
        const auto order = index < actual.normalized_trace.size() ? actual.normalized_trace[index].logical_order : expected.normalized_trace[index].logical_order;
        for (std::size_t candidate = 0; candidate < descriptor.steps.size(); ++candidate)
            if (descriptor.steps[candidate].logical_order <= order / 1024) step = candidate;
    }
    return ScenarioFailure{descriptor.scenario_id, descriptor.seed_set, step, expected.semantic_digest,
                           actual.semantic_digest, std::move(first), "semantic state or normalized trace diverged"};
}

inline std::string FailureText(const ScenarioFailure& failure)
{
    std::ostringstream out;
    out << "scenario=" << failure.scenario_id << " seeds=";
    for (auto seed : failure.seed_set) out << seed << ',';
    out << " step=" << failure.step_index << " expected=" << failure.expected_digest
        << " actual=" << failure.actual_digest << " first_trace=" << failure.first_differing_trace_event
        << " cause=" << failure.message;
    return out.str();
}
}
