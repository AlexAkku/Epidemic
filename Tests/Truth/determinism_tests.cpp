#include "goal5_replay.h"
#include "goal5_scenario_self_tests.h"

using namespace goal5_test_utility;
namespace {
struct OwnedReplayDirectory {
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("epidemic_replay_" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  OwnedReplayDirectory() {
    TRUTH_REQUIRE(std::filesystem::create_directory(path));
  }
  ~OwnedReplayDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};
void RequireEquivalent(const ScenarioDescriptor &descriptor,
                       const ScenarioResult &expected,
                       const ScenarioResult &actual) {
  if (const auto failure = CompareScenario(descriptor, expected, actual))
    throw std::runtime_error(FailureText(*failure));
}
void RealFreshCompositionsAndSaveReloadAreEquivalent() {
  OwnedReplayDirectory directory;
  const auto descriptor = MiniScenario(20261004);
  const auto control =
      ReplayWholeScenario(descriptor, false, directory.path / "control.save");
  const auto fresh =
      ReplayWholeScenario(descriptor, false, directory.path / "fresh.save");
  const auto restored =
      ReplayWholeScenario(descriptor, true, directory.path / "restored.save");
  RequireEquivalent(descriptor, control, fresh);
  RequireEquivalent(descriptor, control, restored);
  TRUTH_REQUIRE(control.step_count == descriptor.steps.size() &&
                control.normalized_trace.size() >= 3);
  const auto different_input = ReplayWholeScenario(
      MiniScenario(20261005), false, directory.path / "different.save");
  TRUTH_REQUIRE(!different_input.failure &&
                different_input.semantic_digest != control.semantic_digest);
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 6 && std::string_view(argv[1]) == "--emit") {
    try {
      TRUTH_REQUIRE(std::string_view(argv[2]) == "mini");
      const std::string_view mode = argv[3];
      TRUTH_REQUIRE(mode == "control" || mode == "save_reload");
      const auto descriptor = MiniScenario(std::stoull(argv[4]));
      const std::filesystem::path output = argv[5];
      const auto result = ReplayWholeScenario(descriptor, mode == "save_reload",
                                              output.string() + ".generation");
      WriteScenarioJson(output, descriptor, mode, result);
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
  return epidemic::truth_tests::Run(
      "Goal5 deterministic semantic runner",
      {
          {"canonical runner rejects adversarial inputs",
           CanonicalScenarioRunnerRejectsAdversarialInputs},
          {"real independent composition and save reload continuation",
           RealFreshCompositionsAndSaveReloadAreEquivalent},
      });
}
