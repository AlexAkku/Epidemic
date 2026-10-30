#pragma once
#include "demo_game.h"

namespace epidemic::game::editor::example {
// The committed archive is the input to both the editor and regression paths.
// Importing its definition starts a new authoring session, without its history.
inline foundation::Result<ProjectDefinition>
DemoDefinition(const std::filesystem::path &destination,
               const ProjectSchema &schema) {
  using Result = foundation::Result<ProjectDefinition>;
  std::error_code error;
  if (std::filesystem::equivalent(destination, EPIDEMIC_EDITOR_DEMO_PROJECT,
                                  error) &&
      !error)
    return Result::Failure(foundation::Error::Create(
        "editor.demo_source",
        "Choose a working copy path; the shipped EditorDemo is protected."));
  auto source = EditorProject::Open(EPIDEMIC_EDITOR_DEMO_PROJECT, schema);
  if (!source)
    return Result::Failure(source.GetError());
  return Result::Success(source.Value()->Snapshot()->definition);
}
} // namespace epidemic::game::editor::example
