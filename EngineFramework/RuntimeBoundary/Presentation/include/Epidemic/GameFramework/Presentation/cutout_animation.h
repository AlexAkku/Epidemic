#pragma once
#include <algorithm>
#include <cmath>
#include <span>
#include <stdexcept>
#include <vector>

namespace epidemic::gameplay::presentation {
// Engine-side 2D cutout animation: local tracks, parent hierarchy, looped sampling.
// No game entities, file parsing, platform handles or mock poses enter this API.
struct CutoutBone {
    int parent = -1;
    double x = 0, y = 0, angle = 0;
};

struct CutoutKey {
    double phase = 0, angle = 0, x = 0, y = 0;
};

using CutoutTrack = std::span<const CutoutKey>;

struct CutoutPose {
    double x = 0, y = 0, angle = 0;
};

inline CutoutKey SampleCutoutTrack(CutoutTrack track, double phase) {
    if (track.empty())
        return {};

    if (track.size() == 1 || phase <= track.front().phase)
        return track.front();
    if (phase >= track.back().phase)
        return track.back();
    auto hi = std::upper_bound(track.begin(), track.end(), phase,
                               [](double v, const auto &key) { return v < key.phase; });
    const auto &a = *(hi - 1), &b = *hi;
    const double length = b.phase - a.phase;
    if (length <= 0)
        throw std::invalid_argument("Cutout keys must be ordered");
    double t = (phase - a.phase) / length;
    t = t * t * (3 - 2 * t);
    return {phase, a.angle + (b.angle - a.angle) * t, a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};

}

inline CutoutKey SampleSmoothCutoutTrack(CutoutTrack track, double phase) {
    if (track.size() < 3)
        return SampleCutoutTrack(track, phase);
    const auto hi = std::upper_bound(track.begin(), track.end(), phase,
                                     [](double v, const auto &key) { return v < key.phase; });
    const std::size_t right = hi == track.end() ? track.size() - 1
                                                : static_cast<std::size_t>(hi - track.begin()),
                      left = right ? right - 1 : 0;
    const auto &a = track[left], &b = track[right];
    const double length = b.phase - a.phase;
    if (length <= 0)
        return a;
    auto slope = [&](std::size_t index, double CutoutKey::*value) {
        const auto last = track.size() - 1, prev = index ? index - 1 : last - 1,
                   next = index == last ? 1 : index + 1;
        const double pa = track[prev].phase - (index == 0 ? 1 : 0),
                     pb = track[next].phase + (index == last ? 1 : 0);
        return (track[next].*value - track[prev].*value) / (pb - pa);
    };

    const double t = std::clamp((phase - a.phase) / length, 0., 1.), t2 = t * t, t3 = t2 * t;
    auto value = [&](double CutoutKey::*member) {
        return (2 * t3 - 3 * t2 + 1) * (a.*member) +
               (t3 - 2 * t2 + t) * length * slope(left, member) + (-2 * t3 + 3 * t2) * (b.*member) +
               (t3 - t2) * length * slope(right, member);
    };

    return {phase, value(&CutoutKey::angle), value(&CutoutKey::x), value(&CutoutKey::y)};

}

inline std::vector<CutoutPose> EvaluateCutoutPose(std::span<const CutoutBone> bones,
                                                  std::span<const CutoutTrack> tracks, double time,
                                                  double duration, double weight = 1,
                                                  std::span<const CutoutPose> additive = {},
                                                  bool smooth = false) {
    if (duration <= 0 || !std::isfinite(time) || !std::isfinite(duration))
        throw std::invalid_argument("Invalid cutout clock");
    double phase = std::fmod(time, duration) / duration;
    if (phase < 0)
        phase += 1;
    weight = std::clamp(weight, 0., 1.);
    std::vector<CutoutPose> result;
    result.reserve(bones.size());
    for (std::size_t i = 0; i < bones.size(); ++i) {
        const auto &bone = bones[i];
        const auto key = i < tracks.size() ? (smooth ? SampleSmoothCutoutTrack(tracks[i], phase)
                                                     : SampleCutoutTrack(tracks[i], phase))
                                           : CutoutKey{};

        CutoutPose local{bone.x + key.x * weight, bone.y + key.y * weight,
                         bone.angle + key.angle * weight};

        if (i < additive.size()) {
            local.x += additive[i].x;
            local.y += additive[i].y;
            local.angle += additive[i].angle;
        }

        if (bone.parent >= 0) {
            if (static_cast<std::size_t>(bone.parent) >= i)
                throw std::invalid_argument("Cutout parent must precede child");
            const auto &p = result[static_cast<std::size_t>(bone.parent)];
            const double a = p.angle * 3.14159265358979323846 / 180;
            local = {p.x + std::cos(a) * local.x - std::sin(a) * local.y,
                     p.y + std::sin(a) * local.x + std::cos(a) * local.y, p.angle + local.angle};

        }

        result.push_back(local);
    }

    return result;
}

// Two-bone foot planting. Bind vectors retain the artwork's native limb axes.
inline void PlantCutoutFoot(std::span<const CutoutBone> bones, std::span<CutoutPose> pose,
                            std::size_t thigh, std::size_t shin, std::size_t foot, double tx,
                            double ty, int bend = 1) {
    if (thigh >= pose.size() || shin >= pose.size() || foot >= pose.size())
        throw std::out_of_range("IK bone index");
    constexpr double rad = 3.14159265358979323846 / 180;
    const double l1 = std::hypot(bones[shin].x, bones[shin].y),
                 l2 = std::hypot(bones[foot].x, bones[foot].y);
    if (l1 < .01 || l2 < .01)
        return;
    const auto hip = pose[thigh];
    const double dx = tx - hip.x, dy = ty - hip.y,
                 dist = std::clamp(std::hypot(dx, dy), std::abs(l1 - l2) + .001, l1 + l2 - .001),
                 direction = std::atan2(dy, dx);
    const double a = direction + bend * std::acos(std::clamp((l1 * l1 + dist * dist - l2 * l2) /
                                                                 (2 * l1 * dist),
                                                             -1., 1.)),
                 kx = hip.x + std::cos(a) * l1, ky = hip.y + std::sin(a) * l1;
    tx = hip.x + std::cos(direction) * dist;
    ty = hip.y + std::sin(direction) * dist;
    pose[thigh].angle = (a - std::atan2(bones[shin].y, bones[shin].x)) / rad;
    pose[shin] = {kx, ky,
                  (std::atan2(ty - ky, tx - kx) - std::atan2(bones[foot].y, bones[foot].x)) / rad};

    pose[foot] = {tx, ty, 0};

}

} // namespace epidemic::gameplay::presentation
