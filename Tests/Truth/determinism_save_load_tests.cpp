#include "goal5_required_scenario_runner.h"
#include "goal5_required_build_stamp.h"

using namespace goal5_test_utility;

namespace {
struct OwnedScenarioDirectory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("epidemic-ms56-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    OwnedScenarioDirectory() {
        TRUTH_REQUIRE(std::filesystem::create_directory(path));
    }

    ~OwnedScenarioDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

};

void RequiredScenarioCompositionBootstrapsSharedOwners() {
    OwnedScenarioDirectory directory;
    const auto descriptor = MiniScenario(20261004);
    WithRequiredScenarioComposition(
        descriptor, directory.path / "scenario.save",
        [&](WholeComposition &engine, auto &services) {
            const auto before = engine.SemanticState().Digest();
            engine.Seed();
            TRUTH_REQUIRE(engine.SemanticState().Digest() != before);
            TRUTH_REQUIRE(
                engine.framework.resources.GetAmount(services.stockpile,
                                                     fw::resources::ResourceTypeId::FromString(
                                                         "ms5.iron")) == 10);
            const auto profiles = engine.framework.progression.CaptureSnapshot().profiles;
            TRUTH_REQUIRE(!profiles.empty());
            TRUTH_REQUIRE(
                engine.framework.progression
                    .GetAttribute(profiles.front().subject,
                                  fw::progression::AttributeTypeId::FromString("ms5.attack")));
            TRUTH_REQUIRE(engine.SemanticState().Count() == 49);
        });
}

void RequiredScenarioMatrixHasEquivalentContinuation() {
    OwnedScenarioDirectory directory;
    for (char name = 'A'; name <= 'H'; ++name) {
        const auto descriptor = RequiredScenario(name);
        const auto control =
            ReplayRequiredScenario(descriptor, false,
                                   directory.path / (std::string(1, name) + ".control"));
        if (control.failure)
            throw std::runtime_error("control: " + FailureText(*control.failure));
        const auto restored =
            ReplayRequiredScenario(descriptor, true,
                                   directory.path / (std::string(1, name) + ".restored"));
        if (restored.failure)
            throw std::runtime_error("restored: " + FailureText(*restored.failure));
        TRUTH_REQUIRE(control.semantic_digest == restored.semantic_digest);
        TRUTH_REQUIRE(control.event_trace_digest == restored.event_trace_digest);
        TRUTH_REQUIRE(control.normalized_trace == restored.normalized_trace);
        for (unsigned repetition = 1; repetition != 10; ++repetition) {
            const auto repeated =
                ReplayRequiredScenario(descriptor, false,
                                       directory.path / (std::string(1, name) + ".repeat." +
                                                         std::to_string(repetition)));
            if (const auto failure = CompareScenario(descriptor, control, repeated))
                throw std::runtime_error("D1 run=" + std::to_string(repetition) + ": " +
                                         FailureText(*failure));
        }

    }

}

} // namespace

void DeterministicBudgetPartitionsReachCommonCompletion() {
    OwnedScenarioDirectory directory;
    const auto small = RequiredScenario('D');
    auto large = small;
    bool first = true;
    std::erase_if(large.steps, [&](ScenarioStep &step) {
        if (step.kind != ScenarioStepKind::ApplyBudget)
            return false;
        if (!first)
            return true;
        first = false;
        step.value = 16;
        step.operation_id = "work.large";
        return false;
    });
    const auto partitioned = ReplayRequiredScenario(small, false, directory.path / "small.save");
    const auto unpartitioned = ReplayRequiredScenario(large, false, directory.path / "large.save");
    TRUTH_REQUIRE(!partitioned.failure && !unpartitioned.failure);
    TRUTH_REQUIRE(partitioned.semantic_digest == unpartitioned.semantic_digest);
}

int main(int argc, char **argv) {
    if (argc == 6 && std::string_view(argv[1]) == "--scenario") {
        try {
            TRUTH_REQUIRE(std::string_view(argv[2]).size() == 1 && argv[2][0] >= 'A' &&
                          argv[2][0] <= 'H');
            const std::string_view mode = argv[3];
            TRUTH_REQUIRE(mode == "control" || mode == "reload");
            const auto descriptor = RequiredScenario(argv[2][0]);
            const auto result = ReplayRequiredScenario(descriptor, mode == "reload", argv[4]);
            WriteScenarioJson(argv[5], descriptor, mode, result, RequiredBuildSourceSha256);
            if (result.failure) {
                std::cerr << FailureText(*result.failure) << '\n';
                return 1;
            }

            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }

    }

    if (argc != 1)
        return 2;
    return epidemic::truth_tests::Run("Goal5 required deterministic scenarios",
                                      {
                                          {"required shared owner composition bootstrap",
                                           RequiredScenarioCompositionBootstrapsSharedOwners},
                                          {"required A-H control and fresh continuation",
                                           RequiredScenarioMatrixHasEquivalentContinuation},
                                          {"deterministic budget partitions share completion",
                                           DeterministicBudgetPartitionsReachCommonCompletion},
                                      });
}
