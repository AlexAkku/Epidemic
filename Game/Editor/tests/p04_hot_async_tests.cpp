#include "../example/demo_game.h"
#include "Epidemic/Game/Editor/content_hot_reload.h"
#include "Epidemic/Game/Editor/png_import.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace ed = epidemic::game::editor;
namespace ex = ed::example;
namespace fs = std::filesystem;
void Check(bool yes, const char *text) {
  if (!yes)
    throw std::runtime_error(text);
}
int main() {
#ifdef _WIN32
  try {
    const auto root =
        fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} / "p04-async-image";
    std::error_code ec;
    fs::remove_all(root, ec);
    ex::Schema schema;
    auto folder = ex::Take(ed::ProjectFolder::Create(
        root, "Async PNG", ex::Definition(schema), schema.View()));
    fs::create_directories(root / "Content");
    // Known 1x1 RGBA PNG fixture, identical to existing P04 content test.
    constexpr unsigned char red[] = {
        137, 80,  78,  71,  13,  10,  26, 10, 0,  0,   0,   13,  73, 72,
        68,  82,  0,   0,   0,   1,   0,  0,  0,  1,   8,   6,   0,  0,
        0,   31,  21,  196, 137, 0,   0,  0,  13, 73,  68,  65,  84, 120,
        156, 99,  248, 207, 192, 208, 0,  0,  4,  129, 1,   128, 44, 85,
        206, 176, 0,   0,   0,   0,   73, 69, 78, 68,  174, 66,  96, 130};
    constexpr unsigned char blue[] = {
        137, 80, 78, 71,  13,  10,  26, 10, 0,  0,   0,   13,  73, 72,
        68,  82, 0,  0,   0,   1,   0,  0,  0,  1,   8,   6,   0,  0,
        0,   31, 21, 196, 137, 0,   0,  0,  13, 73,  68,  65,  84, 120,
        156, 99, 96, 96,  248, 223, 0,  0,  2,  131, 1,   128, 67, 187,
        248, 3,  0,  0,   0,   0,   73, 69, 78, 68,  174, 66,  96, 130};
    const auto path = root / "Content" / "sprite.png";
    const auto write = [&](auto const &png) {
      std::ofstream out(path, std::ios::binary);
      out.write(reinterpret_cast<const char *>(png), sizeof(png));
      Check(out.good(), "Cannot write PNG");
    };
    write(red);
    const auto id = ex::Take(ed::ImportProjectPng(*folder, "sprite.png"));
    ex::Take(folder->Save());
    const auto before = folder->Sources().front();
    ed::ContentHotReload watch{ed::PrepareCookedPng,
                               std::chrono::milliseconds{50}};
    watch.Reset(*folder);
    write(blue);
    fs::last_write_time(
        path, fs::file_time_type::clock::now() + std::chrono::seconds{8}, ec);
    Check(!ec, "Cannot force updated file mtime");
    bool queued = false, updated = false;
    for (int i = 0; i < 600 && !updated; ++i) {
      const auto events = watch.Poll(*folder);
      for (const auto &e : events) {
        if (e.id == id && e.kind == ed::HotAssetEventKind::Queued)
          queued = true;
        if (e.id == id && e.kind == ed::HotAssetEventKind::Updated)
          updated = true;
        Check(e.kind != ed::HotAssetEventKind::Failed,
              "Unexpected decode failure");
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    Check(queued && updated, "Asynchronous import never completed");
    const auto after = folder->Sources().front();
    Check(after.id == id && after.generation == before.generation + 1 &&
              after.cooked_key != before.cooked_key,
          "Source identity/generation publication broken");
    // Cancellation with an external damage must not advance last-good.
    watch.SetEnabled(false);
    {
      std::ofstream out(path, std::ios::binary);
      out << "broken";
    }
    Check(folder->Sources().front().cooked_key == after.cooked_key,
          "Cancel replaced last-good");
    ex::Take(folder->Save());
    folder.reset();
    write(red); // Actual edit while closed, before loading persisted metadata.
    folder = ex::Take(ed::ProjectFolder::Open(root, schema.View()));
    Check(folder->Sources().front().cooked_key == after.cooked_key,
          "Reopen lost cooked key");
    watch.Reset(*folder);
    watch.SetEnabled(true);
    bool cold_updated = false;
    for (int i = 0; i < 600 && !cold_updated; ++i) {
      for (const auto &event : watch.Poll(*folder))
        cold_updated |= event.kind == ed::HotAssetEventKind::Updated;
      std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    Check(cold_updated,
          "Cold-start source change was silently accepted as baseline");
    const auto until = [&](ed::ContentHotReload &service, auto predicate) {
      const auto deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds{5};
      while (std::chrono::steady_clock::now() < deadline) {
        if (predicate(service.Poll(*folder)))
          return true;
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
      }
      return false;
    };
    const auto updated_event = [&](const auto &events) {
      return std::any_of(events.begin(), events.end(), [&](const auto &e) {
        return e.id == id && e.kind == ed::HotAssetEventKind::Updated;
      });
    };
    const auto same_time = fs::last_write_time(path);
    const auto same_size = fs::file_size(path);
    write(blue);
    fs::last_write_time(path, same_time);
    Check(fs::file_size(path) == same_size,
          "Same-size replacement fixture mismatch");
    Check(until(watch, updated_event),
          "Same-size/same-mtime change was not fingerprinted");
    const auto last_good = folder->Sources().front();
    {
      std::ofstream out(path, std::ios::binary);
      out << "broken";
    }
    Check(until(watch,
                [&](const auto &events) {
                  return std::any_of(
                      events.begin(), events.end(), [&](const auto &e) {
                        return e.id == id &&
                               e.kind == ed::HotAssetEventKind::Failed;
                      });
                }),
          "Failed reimport not diagnosed");
    Check(folder->Sources().front().cooked_key == last_good.cooked_key &&
              folder->Sources().front().generation == last_good.generation,
          "Failed reimport replaced last-good");
    watch.SetEnabled(false);
    Check(until(watch, [&](const auto &) { return !watch.Busy(); }),
          "Disabled watcher did not drain cancellation");
    write(blue);
    watch.SetEnabled(true);
    Check(until(watch,
                [&](const auto &events) {
                  return std::any_of(
                      events.begin(), events.end(), [](const auto &e) {
                        return e.kind == ed::HotAssetEventKind::Recovered;
                      });
                }),
          "Restoring the last-good bytes did not report recovery");
    Check(folder->Sources().front().generation == last_good.generation,
          "Recovery of identical bytes changed generation");
    watch.SetEnabled(false);
    Check(until(watch, [&](const auto &) { return !watch.Busy(); }),
          "Recovery worker did not drain");
    write(red);
    std::atomic_int active{0}, peak{0}, calls{0};
    std::promise<void> entered;
    const auto started = entered.get_future();
    const auto owner = std::this_thread::get_id();
    std::atomic_bool wrong_thread{false};
    {
      ed::ContentHotReload slow{
          [&](const fs::path &root_path, std::string_view relative,
              const std::atomic_bool *cancel, std::string_view previous) {
            const auto count = ++active;
            peak.store(std::max(peak.load(), count));
            if (std::this_thread::get_id() == owner)
              wrong_thread = true;
            if (++calls == 1) {
              entered.set_value();
              std::this_thread::sleep_for(std::chrono::milliseconds{500});
            }
            auto result =
                ed::PrepareCookedPng(root_path, relative, cancel, previous);
            --active;
            return result;
          }};
      slow.Reset(*folder);
      Check(until(slow, [&](const auto &) { return slow.Busy(); }),
            "Controlled worker never queued");
      Check(started.wait_for(std::chrono::seconds{1}) ==
                std::future_status::ready,
            "Controlled worker never entered");
      auto clock = std::chrono::steady_clock::now();
      slow.SetEnabled(false);
      const auto cancel_ms = std::chrono::duration<double, std::milli>(
                                 std::chrono::steady_clock::now() - clock)
                                 .count();
      Check(cancel_ms < 100 && slow.Busy(),
            "Cancel blocked or abandoned its active worker slot");
      clock = std::chrono::steady_clock::now();
      slow.Reset(*folder);
      const auto reset_ms = std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - clock)
                                .count();
      Check(reset_ms < 100, "Reset waited for cancelled decoding");
      ex::Take(folder->MoveSource(id, "renamed.png"));
      slow.SetEnabled(true);
      Check(until(slow, updated_event),
            "Cancelled stale path was not requeued under the new path");
      Check(folder->Sources().front().cooked_key != last_good.cooked_key,
            "Wrong candidate published after move");
      std::cout << "CancelPending_ms=" << cancel_ms << " Reset_ms=" << reset_ms
                << '\n';
    }
    Check(active == 0 && peak == 1 && !wrong_thread,
          "Worker escaped final teardown, ran on owner, or overlapped "
          "cancellation");
    {
      const auto moved = root / "Content/renamed.png";
      {
        std::ofstream out(moved, std::ios::binary);
        out.write(reinterpret_cast<const char *>(blue), sizeof(blue));
      }
      std::promise<void> moving;
      const auto moving_started = moving.get_future();
      std::atomic_int preparations{0};
      ed::ContentHotReload stale{[&](const fs::path &source_root,
                                     std::string_view relative,
                                     const std::atomic_bool *cancel,
                                     std::string_view previous) {
        if (++preparations == 1) {
          moving.set_value();
          std::this_thread::sleep_for(std::chrono::milliseconds{300});
        }
        return ed::PrepareCookedPng(source_root, relative, cancel, previous);
      }};
      stale.Reset(*folder);
      Check(until(stale, [&](const auto &) { return stale.Busy(); }),
            "Stale-path worker never queued");
      Check(moving_started.wait_for(std::chrono::seconds{1}) ==
                std::future_status::ready,
            "Stale-path worker never entered");
      ex::Take(folder->MoveSource(
          id, "final.png")); // No Reset/cancel: completion must detect the
                             // identity mismatch.
      Check(until(stale, updated_event),
            "Stale path was discarded without requeueing its replacement");
      Check(preparations >= 2, "Stale candidate was incorrectly published");
    }
    {
      auto prepared = ex::Take(ed::PrepareCookedPng(root, "final.png"));
      std::error_code locked;
      fs::rename(root / "Content/final.png", root / "Content/blocked.png",
                 locked);
      if (!locked)
        fs::rename(root / "Content/blocked.png", root / "Content/final.png");
      Check(static_cast<bool>(locked),
            "Prepared proof did not pin the source until publication");
      const auto generation = folder->Sources().front().generation;
      Check(!folder->PublishCooked(
                epidemic::runtime::AssetId::FromString("unknown"), prepared),
            "Prepared candidate accepted wrong asset identity");
      ex::Take(folder->PublishCooked(id, prepared));
      Check(folder->Sources().front().generation == generation,
            "Unchanged prepared publication advanced generation");
    }
    {
      auto audit = ex::Take(ed::PrepareCookedPng(
          root, "final.png", nullptr, folder->Sources().front().cooked_key));
      Check(!audit.Rebuilt(),
            "Unchanged fingerprint audit unnecessarily decoded/cooked the PNG");
    }
    {
      ed::ContentHotReload burst{ed::PrepareCookedPng,
                                 std::chrono::milliseconds{50}};
      const auto final = root / "Content/final.png",
                 peer = root / "Content/peer.png";
      const auto put = [](const fs::path &file, const auto &bytes) {
        std::ofstream out(file, std::ios::binary);
        out.write(reinterpret_cast<const char *>(bytes), sizeof(bytes));
        Check(out.good(), "Cannot replace source");
      };
      put(peer, red);
      const auto peer_id = ex::Take(ed::ImportProjectPng(*folder, "peer.png"));
      burst.Reset(*folder);
      for (int i = 0; i < 8; ++i)
        put(final, red);
      put(peer, blue);
      bool first = false, second = false;
      Check(until(burst,
                  [&](const auto &events) {
                    for (const auto &e : events)
                      if (e.kind == ed::HotAssetEventKind::Updated) {
                        first |= e.id == id;
                        second |= e.id == peer_id;
                      }
                    return first && second;
                  }),
            "Burst did not update both assets fairly");
      const auto good = folder->Sources().front();
      {
        std::ofstream out(final, std::ios::binary);
        out << "broken";
      }
      put(peer, red);
      first = false;
      second = false;
      Check(until(burst,
                  [&](const auto &events) {
                    for (const auto &e : events) {
                      first |=
                          e.id == id && e.kind == ed::HotAssetEventKind::Failed;
                      second |= e.id == peer_id &&
                                e.kind == ed::HotAssetEventKind::Updated;
                    }
                    return first && second;
                  }),
            "Failed source starved an independent queued source");
      Check(folder->Sources().front().cooked_key == good.cooked_key,
            "Failed burst changed last-good");
      burst.SetEnabled(false);
      Check(until(burst, [&](const auto &) { return !burst.Busy(); }),
            "Burst cancellation did not drain");
      fs::rename(final, root / "Content/final.missing");
      burst.SetEnabled(true);
      first = false;
      Check(until(burst,
                  [&](const auto &events) {
                    return std::any_of(
                        events.begin(), events.end(), [&](const auto &e) {
                          return e.id == id &&
                                 e.kind == ed::HotAssetEventKind::Missing;
                        });
                  }),
            "Rename was not reported as missing");
      put(final, red);
      Check(until(burst,
                  [&](const auto &events) {
                    return std::any_of(
                        events.begin(), events.end(), [&](const auto &e) {
                          return e.id == id &&
                                 e.kind == ed::HotAssetEventKind::Recovered;
                        });
                  }),
            "Replacement with last-good bytes did not recover");
      const auto cooked =
          root / ".epidemic/Cache/images" / (good.cooked_key + ".image");
      fs::rename(cooked, root / ".epidemic/Cache/images" /
                             (good.cooked_key + ".unavailable"));
      ex::Take(burst.Request(*folder, id));
      // Busy is global: a peer audit may start in the publication Poll.
      Check(until(burst,
                  [&](const auto &events) {
                    return fs::exists(cooked) &&
                           std::any_of(
                               events.begin(), events.end(),
                               [&](const auto &e) {
                                 return e.id == id &&
                                        e.kind ==
                                            ed::HotAssetEventKind::Recovered;
                               });
                  }),
            "Explicit reimport did not rebuild/publish/notify cooked data");
      Check(folder->Sources().front().cooked_key == good.cooked_key &&
                folder->Sources().front().generation == good.generation,
            "Force reimport changed the stable last-good generation");
    }
    ex::Take(folder->Save());
    std::cout << "cold_start same_attributes failed_last_good cancel_reset "
                 "stale_move worker_lifetime PASS\n";
    std::cout << "P04 async PNG watcher PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
  } catch (...) {
    std::fputs("Unexpected P04 test failure\n", stderr);
    return 1;
  }
#else
  return 0; // Windows WIC qualification is registered only on Windows.
#endif
}
