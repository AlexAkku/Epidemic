#pragma once
#include "goal5_whole_composition.h"
#include "goal5_scenario.h"
#include <fstream>

namespace goal5_test_utility {
inline ScenarioDescriptor MiniScenario(std::uint64_t seed) {
    return {"scenario.mini",
            "whole.headless.v1",
            {seed},
            "whole.empty.v1",
            {{ScenarioStepKind::SubmitCommand, 1, 0, 0, 0, "whole.seed"},
             {ScenarioStepKind::AdvanceGameTime, 2, 0, 0,
              static_cast<std::int64_t>(333333 + seed % 10000), "time.advance"},
             {ScenarioStepKind::SaveReload, 3, 0, 0, 0, "checkpoint.1"},
             {ScenarioStepKind::CompleteExternalOperation, 4, 0, 0, 1, "scheduled.progression"},
             {ScenarioStepKind::Tick, 5, 0, 0, 1, "scheduled.dispatch"},
             {ScenarioStepKind::Reconcile, 6, 0, 0, 0, "integration.reconcile"},
             {ScenarioStepKind::Materialize, 7, 0, 0, 0, "runtime.materialize"}},
            "D1_D2_CONTROL_SAVE_RELOAD_EQUIVALENT"};

}

class WholeScenarioPort {
  public:
    WholeScenarioPort(WholeComposition &engine, bool checkpoint)
        : engine_(engine), checkpoint_(checkpoint) {
    }

    std::string Digest() {
        return engine_.SemanticState().Digest();
    }

    void Apply(const ScenarioStep &step, SemanticTrace &trace) {
        TRUTH_REQUIRE(engine_.CanTick());
        const auto before = Digest();
        switch (step.kind) {
        case ScenarioStepKind::SubmitCommand:
            if (step.operation_id != "whole.seed")
                throw std::runtime_error("unknown semantic command");
            engine_.Seed();
            break;
        case ScenarioStepKind::AdvanceGameTime:
            TRUTH_REQUIRE(engine_.Advance(std::chrono::microseconds{step.value}));
            engine_.SynchronizeAndReconcile();
            break;
        case ScenarioStepKind::Tick: {
            const auto before_commits = engine_.action_commits;
            const auto dispatched =
                engine_.framework.scheduled.DispatchPending(static_cast<std::uint64_t>(step.value));
            TRUTH_REQUIRE(dispatched.handler_failures == 0);
            if (engine_.action_ready && dispatched.pending == 0)
                TRUTH_REQUIRE(engine_.action_commits ==
                              before_commits + (before_commits == 0 ? 1 : 0));
            break;
        }

        case ScenarioStepKind::CompleteExternalOperation:
            if (step.operation_id != "scheduled.progression")
                throw std::runtime_error("unknown external operation");
            engine_.action_ready = step.value != 0;
            break;
        case ScenarioStepKind::ApplyBudget:
            // DispatchPending takes a deterministic attempt-count budget.
            (void)engine_.framework.scheduled.DispatchPending(
                static_cast<std::uint64_t>(step.value));
            break;
        case ScenarioStepKind::SaveReload:
            if (checkpoint_)
                TRUTH_REQUIRE(engine_.saver.CaptureAndCommit({}));
            break;
        case ScenarioStepKind::Reconcile:
            engine_.SynchronizeAndReconcile();
            break;
        case ScenarioStepKind::Materialize:
            engine_.MaterializePersistentObjects();
            break;
        case ScenarioStepKind::StreamOut: {
            const auto snapshot = rt::CaptureRuntimePersistenceSnapshot(
                *engine_.runtime->services.persistence->query);
            TRUTH_REQUIRE(snapshot);
            for (const auto &record : snapshot.Value().objects) {
                const auto subject =
                    fw::entities::EntityService::ToGameplayObjectRef(DecodeEntityIdentity(record));
                const auto binding = engine_.bridge->GetBinding(subject);
                TRUTH_REQUIRE(binding &&
                              engine_.bridge->Enqueue(
                                  rb::DematerializeRequest{subject, binding->generation}));
            }

            const auto processed = engine_.bridge->Process();
            TRUTH_REQUIRE(processed.failed == 0 &&
                          engine_.bridge->GetDiagnostics().active_bindings == 0);
            break;
        }

        case ScenarioStepKind::StreamIn:
            engine_.MaterializePersistentObjects();
            break;
        }

        const auto after = Digest();
        // Qualification observer emits a logical semantic commit only when
        // authoritative state changed. Storage/retry/projection identity is not
        // a semantic event; the payload is the complete normalized state change.
        if (before != after) {
            const auto entities = engine_.framework.entities.CaptureSnapshot();
            TRUTH_REQUIRE(!entities.records.empty());
            const auto id = entities.records.front().id;
            trace.Commit({"semantic.logical_commit", id.High(), id.Low(), 0, 0,
                          before + "->" + after, step.logical_order * 1024 + 1});
        }

    }

  private:
    WholeComposition &engine_;
    bool checkpoint_;
};

inline ScenarioResult ReplayWholeScenario(const ScenarioDescriptor &descriptor, bool reload,
                                          const std::filesystem::path &storage) {
    SemanticTrace trace;
    ScenarioResult result;
    std::size_t prefix_count = 0;
    std::size_t begin = 0;
    try {
        ValidateScenario(descriptor);
        if (!reload) {
            WithWholeComposition(storage, [&](WholeComposition &engine) {
                WholeScenarioPort port(engine, false);
                result = RunScenario(descriptor, port, trace);
            });
            return result;
        }

        bool fresh = false;
        while (begin < descriptor.steps.size()) {
            auto end = begin;
            while (end < descriptor.steps.size() &&
                   descriptor.steps[end].kind != ScenarioStepKind::SaveReload)
                ++end;
            if (end < descriptor.steps.size())
                ++end;
            WithWholeComposition(storage, [&](WholeComposition &engine) {
                if (fresh) {
                    const auto restored = engine.Restore();
                    if (!restored)
                        throw std::runtime_error("fresh replay restore: " +
                                                 restored.GetError().code);
                    TRUTH_REQUIRE(engine.restorer.CanResume());
                    TRUTH_REQUIRE(engine.SemanticState().Digest() == result.semantic_digest);
                }

                WholeScenarioPort port(engine, true);
                result = RunScenario(descriptor, port, trace, begin, end);
            });
            if (result.failure)
                return result;
            prefix_count += result.step_count;
            result.step_count = prefix_count;
            begin = end;
            fresh = true;
        }

    } catch (const std::exception &error) {
        result.failure = ScenarioFailure{descriptor.scenario_id,
                                         descriptor.seed_set,
                                         begin,
                                         result.semantic_digest.empty() ? "<no completed reference>"
                                                                        : result.semantic_digest,
                                         "<fresh restore failed>",
                                         "<fresh composition boundary>",
                                         error.what()};

    }

    return result;
}

inline std::string JsonString(std::string_view value) {
    std::ostringstream out;
    out << '"';
    for (auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (character == '"' || character == '\\')
            out << '\\' << character;
        else if (byte < 32)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << static_cast<unsigned>(byte) << std::dec;
        else
            out << character;
    }

    out << '"';
    return out.str();
}

inline void WriteScenarioJson(const std::filesystem::path &path,
                              const ScenarioDescriptor &descriptor, std::string_view mode,
                              const ScenarioResult &result,
                              std::string_view build_source_sha256 = {}) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("cannot open owned scenario result artifact");
    out << "{\"scenario_id\":" << JsonString(descriptor.scenario_id)
        << ",\"config_id\":" << JsonString(descriptor.config_id) << ",\"mode\":" << JsonString(mode)
        << ",\"seed_set\":[";
    for (std::size_t index = 0; index < descriptor.seed_set.size(); ++index) {
        if (index)
            out << ',';
        out << descriptor.seed_set[index];
    }

    out << "],\"descriptor_hex\":\"";
    for (auto byte : EncodeScenario(descriptor))
        out << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<unsigned>(std::to_integer<std::uint8_t>(byte));
    out << std::dec << "\",\"semantic_digest\":" << JsonString(result.semantic_digest)
        << ",\"event_trace_digest\":" << JsonString(result.event_trace_digest)
        << ",\"step_count\":" << result.step_count << ",\"normalized_trace\":[";
    for (std::size_t index = 0; index < result.normalized_trace.size(); ++index) {
        if (index)
            out << ',';
        const auto &event = result.normalized_trace[index];
        out << "{\"event_type\":" << JsonString(event.event_type) << ",\"subject\":["
            << event.subject_high << ',' << event.subject_low << "],\"object\":["
            << event.object_high << ',' << event.object_low
            << "],\"semantic_payload\":" << JsonString(event.semantic_payload)
            << ",\"logical_order\":" << event.logical_order << '}';
    }

    out << "],\"status\":" << JsonString(result.failure ? "FAIL" : "PASS") << ",\"failure\":";
    if (result.failure)
        out << JsonString(FailureText(*result.failure));
    else
        out << "null";
    if (!build_source_sha256.empty())
        out << ",\"build_source_sha256\":" << JsonString(build_source_sha256);
    out << "}\n";
    out.close();
    if (!out)
        throw std::runtime_error("scenario result artifact write/close failed");
}

} // namespace goal5_test_utility
