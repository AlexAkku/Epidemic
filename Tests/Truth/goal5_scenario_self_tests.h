#pragma once
#include "goal5_scenario.h"

namespace goal5_test_utility {
inline void CanonicalScenarioRunnerRejectsAdversarialInputs() {
    const ScenarioDescriptor descriptor{"runner.selftest",
                                        "headless.v1",
                                        {20261004},
                                        "whole.empty.v1",
                                        {{ScenarioStepKind::SubmitCommand, 1, 0, 0, 7, "command"},
                                         {ScenarioStepKind::CompleteExternalOperation, 2, 0, 0, 1,
                                          "external.operation"}},
                                        "D1_D2_EXACT"};

    TRUTH_REQUIRE(EncodeScenario(DecodeScenario(EncodeScenario(descriptor))) ==
                  EncodeScenario(descriptor));
    auto rejected = [](auto &&operation) {
        bool failed = false;
        try {
            operation();
        } catch (const std::runtime_error &) {
            failed = true;
        }

        TRUTH_REQUIRE(failed);
    };

    auto clock = descriptor;
    clock.clock_policy = static_cast<ScenarioClockPolicy>(2);
    rejected([&] { ValidateScenario(clock); }); // ambient wall-clock
    auto random = descriptor;
    random.seed_policy = static_cast<ScenarioSeedPolicy>(2);
    rejected([&] { ValidateScenario(random); }); // random_device/ambient seed
    auto floating = descriptor;
    floating.floating_policy = static_cast<ScenarioFloatPolicy>(2);
    rejected([&] { ValidateScenario(floating); }); // native float bytes
    auto tie = descriptor;
    tie.steps[1].logical_order = tie.steps[0].logical_order;
    rejected([&] { ValidateScenario(tie); }); // missing stable tie breaker
    auto async = descriptor;
    async.steps[1].operation_id.clear();
    rejected([&] { ValidateScenario(async); }); // implicit async completion order
    SemanticTrace trace;
    trace.Commit({"commit", 1, 2, 0, 0, "value", 1024});
    rejected([&] { trace.Commit({"commit", 1, 2, 0, 0, "value", 1024}); }); // duplicate event key
    SemanticContributions ordered, reversed;
    ordered.AddSnapshot("a", std::uint64_t{1});
    ordered.AddSnapshot("b", std::uint64_t{2});
    reversed.AddSnapshot("b", std::uint64_t{2});
    reversed.AddSnapshot("a", std::uint64_t{1});
    TRUTH_REQUIRE(ordered.Digest() == reversed.Digest()); // unordered insertion must canonicalize
    SemanticContributions missing;
    missing.AddSnapshot("a", std::uint64_t{1});
    TRUTH_REQUIRE(missing.Digest() != ordered.Digest()); // missing owner changes digest
    SemanticContributions pointer_polluted;
    pointer_polluted.AddSnapshot("a", std::uint64_t{1});
    pointer_polluted.AddSnapshot("pointer.address", std::uint64_t{991});
    TRUTH_REQUIRE(
        pointer_polluted.Digest() !=
        ordered.Digest()); // pointer pollution is observable; domain registry rejects admission

    struct BadHiddenStatePort {
        std::uint64_t &hidden;

        void Apply(const ScenarioStep &, SemanticTrace &) {
            ++hidden;
        }

        std::string Digest() const {
            SemanticContributions d;
            d.AddSnapshot("semantic", hidden);
            return d.Digest();
        }

    };

    std::uint64_t hidden = 0;
    BadHiddenStatePort bad{hidden};

    SemanticTrace first_trace, second_trace;
    const auto first = RunScenario(descriptor, bad, first_trace);
    const auto second = RunScenario(descriptor, bad, second_trace);
    const auto divergence = CompareScenario(descriptor, first, second);
    TRUTH_REQUIRE(divergence && !divergence->expected_digest.empty() &&
                  !divergence->actual_digest.empty());
    TRUTH_REQUIRE(!divergence->first_differing_trace_event.empty() &&
                  FailureText(*divergence).find("runner.selftest") != std::string::npos);
    SemanticContributions positive_zero, negative_zero;
    positive_zero.AddSnapshot("float", 0.0);
    negative_zero.AddSnapshot("float", -0.0);
    TRUTH_REQUIRE(positive_zero.Digest() == negative_zero.Digest());
    rejected([&] {
        CanonicalSemanticWriter writer;
        writer.SemanticFloat(std::numeric_limits<double>::quiet_NaN());
    });
}

} // namespace goal5_test_utility
