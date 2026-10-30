#include <Epidemic/GameFramework/Presentation/presentation.h>
#include <Epidemic/GameFramework/Presentation/startup_sequence.h>
#include <algorithm>
namespace epidemic::gameplay::presentation {
namespace {
struct SteelGlyph {
  double width;
  std::vector<Point> face;
  std::vector<std::vector<Point>> holes;
};
// Custom cut steel lettering. Counters are genuine empty apertures, not font
// strokes.
SteelGlyph Letter(wchar_t c) {
  switch (c) {
  case L' ':
    return {44, {}, {}};
  case L'A':
    return {74,
            {{0, 100},
             {25, 0},
             {49, 0},
             {74, 100},
             {53, 100},
             {48, 77},
             {26, 77},
             {21, 100}},
            {{{31, 58}, {43, 58}, {37, 27}}}};
  case L'W':
    return {92,
            {{0, 0},
             {21, 0},
             {29, 65},
             {36, 23},
             {56, 23},
             {63, 65},
             {71, 0},
             {92, 0},
             {77, 100},
             {56, 100},
             {46, 58},
             {36, 100},
             {15, 100}},
            {}};
  case L'T':
    return {72,
            {{0, 0},
             {72, 0},
             {72, 20},
             {46, 20},
             {46, 100},
             {26, 100},
             {26, 20},
             {0, 20}},
            {}};
  case L'H':
    return {74,
            {{0, 0},
             {20, 0},
             {20, 39},
             {54, 39},
             {54, 0},
             {74, 0},
             {74, 100},
             {54, 100},
             {54, 59},
             {20, 59},
             {20, 100},
             {0, 100}},
            {}};
  case L'E':
    return {68,
            {{7, 0},
             {68, 0},
             {68, 18},
             {20, 18},
             {20, 41},
             {58, 41},
             {58, 59},
             {20, 59},
             {20, 82},
             {68, 82},
             {68, 100},
             {0, 100},
             {0, 7}},
            {}};
  case L'P':
    return {70,
            {{7, 0},
             {57, 0},
             {70, 13},
             {70, 52},
             {57, 65},
             {20, 65},
             {20, 100},
             {0, 100},
             {0, 7}},
            {{{20, 18}, {48, 18}, {51, 21}, {51, 44}, {48, 47}, {20, 47}}}};
  case L'I':
    return {32, {{0, 0}, {32, 0}, {32, 100}, {0, 100}}, {}};
  case L'D':
    return {74,
            {{0, 0}, {55, 0}, {74, 19}, {74, 81}, {55, 100}, {0, 100}},
            {{{20, 19}, {45, 19}, {54, 28}, {54, 72}, {45, 81}, {20, 81}}}};
  case L'M':
    return {86,
            {{0, 100},
             {0, 0},
             {20, 0},
             {43, 40},
             {66, 0},
             {86, 0},
             {86, 100},
             {66, 100},
             {66, 37},
             {43, 74},
             {20, 37},
             {20, 100}},
            {}};
  case L'C':
    return {72,
            {{18, 0},
             {60, 0},
             {72, 12},
             {72, 25},
             {51, 25},
             {46, 19},
             {26, 19},
             {20, 25},
             {20, 75},
             {26, 81},
             {46, 81},
             {51, 75},
             {72, 75},
             {72, 88},
             {60, 100},
             {18, 100},
             {0, 82},
             {0, 18}},
            {}};
  case L'N':
    return {74,
            {{0, 100},
             {0, 0},
             {20, 0},
             {54, 61},
             {54, 0},
             {74, 0},
             {74, 100},
             {54, 100},
             {20, 39},
             {20, 100}},
            {}};
  case L'G':
    return {74,
            {{18, 0},   {61, 0},  {74, 13}, {74, 27}, {53, 27}, {48, 19},
             {26, 19},  {20, 25}, {20, 75}, {26, 81}, {49, 81}, {54, 76},
             {54, 61},  {38, 61}, {38, 43}, {74, 43}, {74, 82}, {56, 100},
             {18, 100}, {0, 82},  {0, 18}},
            {}};
  default:
    return {68, {}, {}};
  }
}
double WordWidth(std::wstring_view word, double tracking) {
  double w = 0;
  for (auto c : word)
    w += Letter(c).width + tracking;
  return w - tracking;
}
} // namespace
void Canvas::DrawStartup(double age, bool motion) {
  const auto line = [&](double x, double y, double x2, double y2,
                        unsigned color, double width, double alpha) {
    Line(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x2),
         static_cast<float>(y2), color, static_cast<float>(width),
         static_cast<float>(alpha));
  };
  const auto e = LogicalExtent();
  const auto state = StartupSequence::At(age, motion);
  const auto client = ClientExtent();
  PushTransform(Matrix::Translation(-(e.x - std::min(1280.f, client.x)) * .5f,
                                    -(e.y - 720) * .5f));
  Rect(0, 0, e.x, e.y, 0x000000);
  if (state.logoOpacity <= 0) {
    PopTransform();
    return;
  }
  const std::wstring title = L"EPIDEMIC ENGINE", caption = L"MADE WITH";
  const double scale = std::min(e.x * .82 / WordWidth(title, 13), e.y / 510.);
  const double top = e.y * .5 - 105 * scale,
               left = (e.x - WordWidth(title, 13) * scale) * .5;
  const double light = (e.x * .04 + (e.x * .96 - e.x * .04) * state.sweep),
               band = 80 * scale;
  // The illumination belongs to the metal; the surrounding stage remains black.
  auto word = [&](std::wstring_view text, double x, double y, double z,
                  double tracking) {
    for (wchar_t c : text) {
      const auto glyph = Letter(c);
      if (glyph.face.empty()) {
        x += (glyph.width + tracking) * z;
        continue;
      }
      auto map = [&](const std::vector<Point> &points) {
        std::vector<Point> out;
        out.reserve(points.size());
        for (auto p : points)
          out.push_back({static_cast<float>(x + p.x * z),
                         static_cast<float>(y + p.y * z)});
        return out;
      };
      const auto face = map(glyph.face);
      const double cx = x + glyph.width * z * .5,
                   lit = motion ? std::exp(-std::pow(
                                      (cx - light) / (band * 2.2), 2.))
                                : 0;
      const double steel =
          motion ? .015 + .92 * lit *
                              (1 - StartupSequence::Smooth((age - 2.15) / .45))
                 : 0;
      ClipPolygon(face);
      Gradient(static_cast<float>(x - 3), static_cast<float>(y - 3),
               static_cast<float>(glyph.width * z + 6),
               static_cast<float>(106 * z), 0xa6b2bc, 0x26303b,
               static_cast<float>(steel * state.logoOpacity));
      // Brushed grain, machining scars and a travelling diagonal specular
      // reflection.
      for (int grain = 0; grain < 22; ++grain) {
        const double yy = y + (grain * 4.57 + 1) * z;
        line(x, yy, x + glyph.width * z, yy, grain % 4 ? 0x18232d : 0xd8e4e9,
             .45 * z, steel * .17 * state.logoOpacity);
      }
      if (motion && age < 2.65) {
        for (int strip = 0; strip < 22; ++strip) {
          const double offset = (strip - 10.5) * band / 15,
                       shine = std::exp(-std::pow(offset / (band * .28), 2.));
          const float xx =
              static_cast<float>(light + offset - (y - e.y * .5) * .35);
          Polygon({{xx, static_cast<float>(y - 4)},
                   {xx + static_cast<float>(band / 15 + 1),
                    static_cast<float>(y - 4)},
                   {xx - static_cast<float>(38 * z) +
                        static_cast<float>(band / 15 + 1),
                    static_cast<float>(y + 105 * z)},
                   {xx - static_cast<float>(38 * z),
                    static_cast<float>(y + 105 * z)}},
                  0xf0f5f7,
                  static_cast<float>(shine * .85 * state.logoOpacity));
        }
      }
      UnclipPolygon();
      auto edges = [&](const std::vector<Point> &p, bool counter) {
        for (std::size_t i = 0; i < p.size(); ++i) {
          const auto a = p[i], b = p[(i + 1) % p.size()];
          const double above =
              std::max(0., static_cast<double>(a.y - b.y) + .55 * (b.x - a.x));
          const double rim = motion ? .22 + .73 * lit : .7;
          line(a.x, a.y, b.x, b.y, above > 0 ? 0xdde8ee : 0xa8bbc7,
               std::max(1.2, z * (counter ? 1.15 : 1.6)),
               state.logoOpacity * rim);
        }
      };
      for (const auto &hole : glyph.holes) {
        const auto points = map(hole);
        Polygon(points, 0x000000);
        edges(points, true);
      }
      edges(face, false);
      x += (glyph.width + tracking) * z;
    }
  };
  word(caption, left, top, scale * .58, 9);
  word(title, left, top + 110 * scale, scale, 13);
  PopTransform();
}
} // namespace epidemic::gameplay::presentation
