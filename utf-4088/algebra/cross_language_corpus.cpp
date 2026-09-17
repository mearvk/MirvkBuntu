#include "glyph8x12.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace utf4088 {

enum class CorpusLanguage : std::uint8_t { AmericanEnglish, Korean, Germanic };

enum class CorpusStage : std::uint8_t { Start, Intermediate, Final };

struct CorpusRecord {
    std::uint32_t id;
    CorpusLanguage language;
    CorpusStage stage;
    std::string_view seed;
    Glyph8x12 glyph;
    GlyphMetrics metrics;
    double resolution_score;
};

static double score(const GlyphMetrics& m) {
    const double density = static_cast<double>(m.black_pixels) / 96.0;
    const double connectivity = m.connected ? 1.0 : 0.0;
    const double topology = std::min(1.0, static_cast<double>(m.edge_count) / 95.0);
    const double complexity = std::min(1.0, static_cast<double>(m.transitions) / 120.0);
    return 0.45 * connectivity + 0.25 * topology +
           0.20 * complexity + 0.10 * (1.0 - std::abs(density - 0.30));
}

// The seeds are intentionally explicit and small. They are candidate corpus
// anchors, not claims that these generated bitmaps reproduce historical type.
static constexpr std::array<std::string_view, 26> ENGLISH = {
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"
};

static constexpr std::array<std::string_view, 24> KOREAN = {
    "ㄱ","ㄴ","ㄷ","ㄹ","ㅁ","ㅂ","ㅅ","ㅇ","ㅈ","ㅊ","ㅋ","ㅌ",
    "ㅍ","ㅎ","ㅏ","ㅑ","ㅓ","ㅕ","ㅗ","ㅛ","ㅜ","ㅠ","ㅡ","ㅣ"
};

static constexpr std::array<std::string_view, 26> GERMANIC = {
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"
};

std::vector<CorpusRecord> build_cross_language_corpus() {
    std::vector<CorpusRecord> out;
    std::uint32_t id = 0;

    const auto append = [&](CorpusLanguage language,
                            std::string_view seed) {
        for (int s = 0; s < 3; ++s) {
            CorpusStage stage = static_cast<CorpusStage>(s);
            std::string stage_seed(seed);
            stage_seed += std::to_string(s);
            auto glyph = seed_glyph(stage_seed);
            auto metrics = analyze_glyph(glyph);
            out.push_back({id++, language, stage, seed, glyph, metrics,
                           score(metrics)});
        }
    };

    for (auto s : ENGLISH) append(CorpusLanguage::AmericanEnglish, s);
    for (auto s : KOREAN) append(CorpusLanguage::Korean, s);
    for (auto s : GERMANIC) append(CorpusLanguage::Germanic, s);
    return out;
}

} // namespace utf4088
