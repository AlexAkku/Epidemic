#pragma once
#include <Epidemic/GameFramework/Presentation/cutout_animation.h>
#include <Epidemic/GameFramework/Presentation/presentation.h>

namespace epidemic::gameplay::presentation {

// Pivots are in source-image pixels; angles follow Canvas degrees, Y down.
// Draw an image at (0,0,width,height) inside this transform. Attach hands,
// weapons/lights using the same transform, rather than a second angle formula.
inline Matrix CutoutSpriteMatrix(const CutoutPose &pose, Point pivot, bool mirrored = false) {
    return Matrix::Translation(-pivot.x, -pivot.y) * Matrix::Scale(mirrored ? -1.f : 1.f, 1.f) *
           Matrix::Rotation(static_cast<float>(pose.angle)) *
           Matrix::Translation(static_cast<float>(pose.x), static_cast<float>(pose.y));
}

inline Point TransformCutoutAnchor(const Matrix &matrix, Point source_pixel) {
    return {source_pixel.x * matrix._11 + source_pixel.y * matrix._21 + matrix._31,
            source_pixel.x * matrix._12 + source_pixel.y * matrix._22 + matrix._32};
}

// Reflect the already evaluated world pose once around the root's X axis.
// Render the reflected pose with mirrored=true to reflect the artwork too.
inline std::vector<CutoutPose> MirrorCutoutPose(std::span<const CutoutPose> pose, double root_x) {
    std::vector<CutoutPose> result(pose.begin(), pose.end());
    for (auto &bone : result) {
        bone.x = 2 * root_x - bone.x;
        bone.angle = -bone.angle;
    }
    return result;
}

// Asset-independent authoring preview: joints and parent links in logical units.
inline void DrawCutoutRigPreview(Canvas &canvas, std::span<const CutoutBone> bones,
                                std::span<const CutoutPose> pose, Point origin = {}, float scale = 1) {
    if (bones.size() != pose.size() || !std::isfinite(scale) || scale <= 0)
        throw std::invalid_argument("Invalid rig preview pose or scale");
    const auto point = [&](std::size_t i) {
        return Point{origin.x + static_cast<float>(pose[i].x) * scale,
                     origin.y + static_cast<float>(pose[i].y) * scale};
    };
    for (std::size_t i = 0; i < bones.size(); ++i) {
        const auto p = point(i);
        if (bones[i].parent >= 0) {
            const auto parent = static_cast<std::size_t>(bones[i].parent);
            if (parent >= i)
                throw std::invalid_argument("Cutout parent must precede child");
            const auto q = point(parent);
            canvas.Line(q.x, q.y, p.x, p.y, 0x61cfff, 2);
        }
        canvas.Circle(p.x, p.y, 4, 0xffc56b);
    }
}
} // namespace epidemic::gameplay::presentation
