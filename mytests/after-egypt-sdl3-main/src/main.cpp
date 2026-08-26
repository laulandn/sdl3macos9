// SPDX-License-Identifier: MIT OR Unlicense
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr int kWindowW = 1180;
constexpr int kWindowH = 760;
constexpr SDL_WindowFlags kWindowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
constexpr double kPi = 3.14159265358979323846;
constexpr double kCampWidth = 600.0;
constexpr double kCampScale = 300.0;
constexpr double kCampZClip = 10.0;
constexpr int kCloudsNum = 16;
constexpr int kCloudPts = 512;
constexpr int kCloudPens = 8;
constexpr int kCloudPenPts = 16;
constexpr int kCloudPenSize = 16;
constexpr int kCloudSamples = 6;

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a = 255;
};

constexpr Color kSky{105, 190, 235};
constexpr Color kDeepSky{54, 132, 220};
constexpr Color kDesert{210, 166, 78};
constexpr Color kRock{100, 93, 86};
constexpr Color kBrown{104, 71, 37};
constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{245, 245, 238};
constexpr Color kPanelLite{37, 52, 78, 255};
constexpr Color kBlue{25, 92, 205};
constexpr Color kWater{47, 190, 240};
constexpr Color kRed{235, 66, 55};
constexpr Color kYellow{255, 224, 83};
constexpr Color kGold{244, 177, 45};
constexpr Color kGreen{72, 188, 116};
constexpr Color kPurple{144, 107, 220};
constexpr Color kInk{22, 20, 18};

double nowSeconds()
{
    static const uint64_t freq = SDL_GetPerformanceFrequency();
    return static_cast<double>(SDL_GetPerformanceCounter()) / static_cast<double>(freq);
}

fs::path findFont()
{
    const std::array<fs::path, 8> candidates = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/opt/homebrew/share/fonts/dejavu/DejaVuSans.ttf",
        "/opt/homebrew/share/fonts/liberation-fonts/LiberationSans-Regular.ttf",
    };
    for (const auto &path : candidates) {
        if (fs::exists(path)) return path;
    }
    throw std::runtime_error("no usable TTF font found");
}

double clamp(double value, double lo, double hi)
{
    return std::max(lo, std::min(value, hi));
}

double wrap(double value, double lo, double hi)
{
    const double span = hi - lo;
    while (value < lo) value += span;
    while (value >= hi) value -= span;
    return value;
}

std::optional<fs::path> findFromAncestors(const fs::path &relative)
{
    fs::path path = fs::current_path();
    for (int i = 0; i < 12; ++i) {
        const fs::path candidate = path / relative;
        if (fs::exists(candidate)) return candidate;
        if (!path.has_parent_path() || path == path.parent_path()) break;
        path = path.parent_path();
    }
    return std::nullopt;
}

std::optional<fs::path> findAfterEgyptAssetsDir()
{
    if (const auto path = findFromAncestors("after-egypt-sdl3/assets/AfterEgypt")) return path;
    if (const auto path = findFromAncestors("assets/AfterEgypt")) return path;
    if (const auto path = findFromAncestors("TinkerOS/Apps/AfterEgypt")) return path;
    return std::nullopt;
}

std::string trim(std::string text)
{
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
    text.erase(std::find_if(text.rbegin(), text.rend(), notSpace).base(), text.end());
    return text;
}

std::string lowerCopy(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string stripDolDoc(std::string text)
{
    std::string out;
    bool inCmd = false;
    for (size_t i = 0; i < text.size(); ++i) {
        if (i + 1 < text.size() && text[i] == '$' && text[i + 1] == '$') {
            inCmd = !inCmd;
            ++i;
            continue;
        }
        if (!inCmd) out.push_back(text[i]);
    }
    return trim(out);
}

std::vector<std::string> readTextLines(const fs::path &path)
{
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

std::string joinLines(const std::vector<std::string> &lines)
{
    std::string out;
    for (const std::string &line : lines) {
        out += line;
        out += '\n';
    }
    return out;
}

class TextAssets {
public:
    void load()
    {
        if (const auto path = findFromAncestors("after-egypt-sdl3/assets/Bible.TXT")) {
            bibleLines_ = readTextLines(*path);
            biblePath_ = *path;
        } else if (const auto path = findFromAncestors("assets/Bible.TXT")) {
            bibleLines_ = readTextLines(*path);
            biblePath_ = *path;
        } else if (const auto path = findFromAncestors("TempleOS/Misc/Bible.TXT")) {
            bibleLines_ = readTextLines(*path);
            biblePath_ = *path;
        } else if (const auto path = findFromAncestors("TinkerOS/Misc/Bible.TXT")) {
            bibleLines_ = readTextLines(*path);
            biblePath_ = *path;
        }

        if (const auto path = findFromAncestors("after-egypt-sdl3/assets/God/Vocab.DD")) {
            loadVocab(*path);
        } else if (const auto path = findFromAncestors("assets/God/Vocab.DD")) {
            loadVocab(*path);
        } else if (const auto path = findFromAncestors("TinkerOS/Adam/God/Vocab.DD")) {
            loadVocab(*path);
        } else if (const auto path = findFromAncestors("TempleOS/Adam/God/Vocab.DD")) {
            loadVocab(*path);
        }

        if (const auto path = findFromAncestors("after-egypt-sdl3/assets/God/HSNotes.DD")) {
            loadHelp(*path);
        } else if (const auto path = findFromAncestors("assets/God/HSNotes.DD")) {
            loadHelp(*path);
        } else if (const auto path = findFromAncestors("TinkerOS/Adam/God/HSNotes.DD")) {
            loadHelp(*path);
        } else if (const auto path = findFromAncestors("TempleOS/Adam/God/HSNotes.DD")) {
            loadHelp(*path);
        }
    }

    bool hasBible() const
    {
        return !bibleLines_.empty();
    }

    std::vector<std::string> bibleVerse(const std::string &book, const std::string &marker,
                                        int lineCount) const
    {
        if (bibleLines_.empty()) {
            return {book + " " + marker, "Bible text asset not found."};
        }

        const int start = bookStart(book);
        if (start < 0) return {book + " " + marker};

        const std::string needle = marker + " ";
        int found = -1;
        for (int i = start; i < static_cast<int>(bibleLines_.size()); ++i) {
            if (i > start && isBookHeading(bibleLines_[static_cast<size_t>(i)])) break;
            const std::string &line = bibleLines_[static_cast<size_t>(i)];
            if (line.rfind(marker, 0) == 0 || line.find(" " + needle) != std::string::npos ||
                line.find("  " + needle) != std::string::npos) {
                found = i;
                break;
            }
        }
        if (found < 0) return {book + " " + marker};

        std::vector<std::string> out;
        out.push_back(book + " " + marker);
        for (int i = found; i < static_cast<int>(bibleLines_.size()) &&
                            static_cast<int>(out.size()) <= lineCount; ++i) {
            std::string line = trim(bibleLines_[static_cast<size_t>(i)]);
            if (line.empty()) {
                if (!out.empty() && !out.back().empty()) out.push_back("");
                continue;
            }
            out.push_back(line);
        }
        return out;
    }

    std::vector<std::string> randomGodText(std::mt19937 &rng) const
    {
        if (!bibleLines_.empty()) {
            std::uniform_int_distribution<int> coin(0, 1);
            if (coin(rng) == 0) {
                std::vector<int> candidates;
                for (int i = 0; i < static_cast<int>(bibleLines_.size()); ++i) {
                    const std::string line = trim(bibleLines_[static_cast<size_t>(i)]);
                    if (!line.empty() && std::isdigit(static_cast<unsigned char>(line.front())) &&
                        line.find(':') != std::string::npos) {
                        candidates.push_back(i);
                    }
                }
                if (!candidates.empty()) {
                    std::uniform_int_distribution<int> pick(0, static_cast<int>(candidates.size()) - 1);
                    const int start = candidates[static_cast<size_t>(pick(rng))];
                    std::vector<std::string> out = {"God Says...", "Bible Passage"};
                    for (int i = start; i < static_cast<int>(bibleLines_.size()) &&
                                        static_cast<int>(out.size()) < 17; ++i) {
                        const std::string line = trim(bibleLines_[static_cast<size_t>(i)]);
                        if (!line.empty()) out.push_back(line);
                    }
                    return out;
                }
            }
        }

        std::vector<std::string> words;
        if (!vocab_.empty()) {
            std::uniform_int_distribution<int> pick(0, static_cast<int>(vocab_.size()) - 1);
            for (int i = 0; i < 16; ++i) words.push_back(vocab_[static_cast<size_t>(pick(rng))]);
        } else {
            words = {"trust", "walk", "wilderness", "water", "cloud", "bread", "mercy", "law",
                     "mountain", "listen", "return", "promise", "camp", "journey", "people", "go"};
        }

        std::vector<std::string> out = {"God Says..."};
        std::string line;
        for (const std::string &word : words) {
            if (line.size() + word.size() + 1 > 46) {
                out.push_back(line);
                line.clear();
            }
            if (!line.empty()) line += ' ';
            line += word;
        }
        if (!line.empty()) out.push_back(line);
        return out;
    }

    std::vector<std::string> helpLines() const
    {
        if (!helpLines_.empty()) return helpLines_;
        return {"The Purpose of Life",
                "Add your own story-line...",
                "Like old school toys."};
    }

private:
    std::vector<std::string> bibleLines_;
    std::vector<std::string> vocab_;
    std::vector<std::string> helpLines_;
    fs::path biblePath_;

    static bool isBookHeading(const std::string &line)
    {
        const std::string lower = lowerCopy(line);
        return lower.find("the ") == 0 && lower.find(" book ") != std::string::npos;
    }

    int bookStart(const std::string &book) const
    {
        const std::string lowerBook = lowerCopy(book);
        for (int i = 0; i < static_cast<int>(bibleLines_.size()); ++i) {
            const std::string lower = lowerCopy(bibleLines_[static_cast<size_t>(i)]);
            if (lower.find(lowerBook) != std::string::npos && isBookHeading(lower)) return i;
        }
        return -1;
    }

    void loadVocab(const fs::path &path)
    {
        for (std::string line : readTextLines(path)) {
            line = trim(line);
            if (line.empty() || line.find('$') != std::string::npos) continue;
            const bool mostlyWord = std::all_of(line.begin(), line.end(), [](unsigned char ch) {
                return std::isalpha(ch) || ch == '\'' || ch == '-';
            });
            if (mostlyWord) vocab_.push_back(line);
        }
    }

    void loadHelp(const fs::path &path)
    {
        for (std::string line : readTextLines(path)) {
            line = stripDolDoc(line);
            if (!line.empty()) helpLines_.push_back(line);
        }
        helpLines_.push_back("");
        helpLines_.push_back("Add your own story-line...");
        helpLines_.push_back("Like old school toys.");
    }
};

struct CampObj {
    double x = 0.0;
    double z = 0.0;
    double dx = 0.0;
    double dz = 0.0;
    bool tent = false;
};

struct CampProjection {
    bool visible = false;
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    double depth = 0.0;
};

struct CloudPoint {
    int x = 0;
    int y = 0;
    uint16_t shade = 0;
};

struct Cloud {
    double x = 0.0;
    double y = 0.0;
    double dx = 0.0;
    double dy = 0.0;
    uint16_t color = 0;
    std::array<CloudPoint, kCloudPts> points;
};

struct CloudPen {
    std::array<SDL_Point, kCloudPenPts> points;
};

struct Quail {
    double x = 0.0;
    double y = 0.0;
    double dx = 0.0;
    double dy = 0.0;
    double phase = 0.0;
    bool dead = false;
};

struct HorebObj {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double dx = 0.0;
    double dz = 0.0;
    double phase = 0.0;
    int bi = 1;
    int seed = 0;
    bool mirror = false;
};

struct HorebProjection {
    bool visible = false;
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    double depth = 0.0;
    double side = 0.0;
};

enum class Mode {
    Camp,
    God,
    Clouds,
    Court,
    Map,
    WaterRock,
    Battle,
    Quail,
    Comics,
    Help,
};

enum class Flow {
    CampWatch,
    Menu,
    Scene,
};

enum class Action {
    BreakCamp,
    God,
    Clouds,
    Court,
    Map,
    WaterRock,
    Battle,
    Quail,
    Comics,
    Help,
    Quit,
};

enum class GodStage {
    Climb,
    Horeb,
    Talking,
};

struct MenuButton {
    SDL_FRect rect;
    std::string label;
    Action action;
};

struct Tone {
    double hz = 440.0;
    double seconds = 0.18;
};

class AudioEngine {
public:
    ~AudioEngine()
    {
        shutdown();
    }

    void init()
    {
        SDL_AudioSpec spec{SDL_AUDIO_F32, 1, sampleRate_};
        stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!stream_) return;
        SDL_ResumeAudioStreamDevice(stream_);
    }

    void shutdown()
    {
        if (stream_) {
            SDL_DestroyAudioStream(stream_);
            stream_ = nullptr;
        }
    }

    void toggleMuted()
    {
        muted_ = !muted_;
        if (muted_ && stream_) SDL_ClearAudioStream(stream_);
    }

    bool muted() const
    {
        return muted_;
    }

    void pump(Mode mode, double)
    {
        if (!stream_ || muted_) return;
        if (!trackActive_ || mode != trackMode_) {
            SDL_ClearAudioStream(stream_);
            trackActive_ = true;
            trackMode_ = mode;
            noteIndex_ = 0;
            noteRemaining_ = 0.0;
            noteStarted_ = false;
            melodyPhase_ = 0.0;
        }

        const int queued = SDL_GetAudioStreamQueued(stream_);
        const int targetBytes = static_cast<int>(sampleRate_ * sizeof(float) * 0.12);
        if (queued >= targetBytes) return;

        constexpr int frames = 2048;
        samples_.assign(frames, 0.0f);
        const std::vector<Tone> &melody = melodyFor(mode);
        if (noteIndex_ >= melody.size()) {
            noteIndex_ = 0;
            noteRemaining_ = 0.0;
        }
        for (int i = 0; i < frames; ++i) {
            const double step = 1.0 / static_cast<double>(sampleRate_);
            if (noteRemaining_ <= 0.0) {
                if (noteStarted_) {
                    noteIndex_ = (noteIndex_ + 1) % melody.size();
                } else {
                    noteStarted_ = true;
                }
                noteRemaining_ = melody[noteIndex_].seconds;
            }
            noteRemaining_ -= step;
            const double hz = melody[noteIndex_].hz;
            melodyPhase_ = wrap(melodyPhase_ + hz * step, 0.0, 1.0);
            double sample = std::sin(melodyPhase_ * 2.0 * kPi) * 0.026;

            samples_[static_cast<size_t>(i)] = static_cast<float>(clamp(sample, -0.25, 0.25));
        }
        SDL_PutAudioStreamData(stream_, samples_.data(),
                               static_cast<int>(samples_.size() * sizeof(float)));
    }

    void silence()
    {
        if (stream_) SDL_ClearAudioStream(stream_);
        trackActive_ = false;
        noteIndex_ = 0;
        noteRemaining_ = 0.0;
        noteStarted_ = false;
        melodyPhase_ = 0.0;
    }

private:
    static double noteFrequency(char note, int octave)
    {
        int semitone = 0;
        switch (note) {
        case 'C': semitone = 0; break;
        case 'D': semitone = 2; break;
        case 'E': semitone = 4; break;
        case 'F': semitone = 5; break;
        case 'G': semitone = 7; break;
        case 'A': semitone = 9; break;
        case 'B': semitone = 11; break;
        default: break;
        }
        const int midi = (octave + 1) * 12 + semitone;
        return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    }

    static double templeDuration(char ch)
    {
        switch (ch) {
        case 'w': return 1.10;
        case 'h': return 0.72;
        case 'q': return 0.36;
        case 'e': return 0.18;
        case 't': return 0.12;
        case 's': return 0.09;
        default: return 0.18;
        }
    }

    static std::vector<Tone> parseTempleSong(std::initializer_list<const char *> lines)
    {
        std::vector<Tone> notes;
        int octave = 4;
        double duration = 0.18;
        for (const char *line : lines) {
            for (const char *p = line; *p; ++p) {
                const char ch = *p;
                if (ch >= '0' && ch <= '8') {
                    octave = ch - '0';
                } else if (ch == 'w' || ch == 'h' || ch == 'q' ||
                           ch == 'e' || ch == 't' || ch == 's') {
                    duration = templeDuration(ch);
                } else if (ch >= 'A' && ch <= 'G') {
                    notes.push_back({noteFrequency(ch, octave), duration});
                }
            }
        }
        if (notes.empty()) notes = {{196.0, 0.18}, {246.94, 0.18}, {293.66, 0.18}, {392.0, 0.18}};
        return notes;
    }

    const std::vector<Tone> &melodyFor(Mode mode) const
    {
        static const std::vector<Tone> afterEgyptSong = parseTempleSong({
            "5eGFqG4etB5EF4eB5EEF4etB5DCeCFqF",
            "5eGFqG4etB5EF4eB5EEF4etB5DCeCFqF",
            "4A5etD4AAqB5D4eB5EqEetECDG4B5G",
            "4qA5etD4AAqB5D4eB5EqEetECDG4B5G",
        });
        static const std::vector<Tone> clouds = parseTempleSong({
            "4qB5etD4AG5qD4sG5E4G5EetCEDqFEeDC",
            "4qB5etD4AG5qD4sG5E4G5EetCEDqFEeDC",
            "5CGqD4eA5DsDCDCqGEetD4A5D4sG5D4G5D",
            "5eCGqD4eA5DsDCDCqGEetD4A5D4sG5D4G5D",
        });
        if (mode == Mode::Clouds) return clouds;
        return afterEgyptSong;
    }

    SDL_AudioStream *stream_ = nullptr;
    int sampleRate_ = 48000;
    bool muted_ = false;
    size_t noteIndex_ = 0;
    double noteRemaining_ = 0.0;
    bool noteStarted_ = false;
    bool trackActive_ = false;
    Mode trackMode_ = Mode::Camp;
    double melodyPhase_ = 0.0;
    std::vector<float> samples_;
};

struct SpriteImage {
    int width = 0;
    int height = 0;
    int originX = 0;
    int originY = 0;
    std::vector<uint32_t> pixels;
};

struct TempleSprite {
    int width = 0;
    int height = 0;
    int originX = 0;
    int originY = 0;
    SDL_Texture *texture = nullptr;
};

SDL_FColor fcolor(Color c)
{
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}

class TempleSprites {
public:
    void load(SDL_Renderer *renderer)
    {
        renderer_ = renderer;
        const std::optional<fs::path> root = findAfterEgyptDir();
        if (!root) return;

        const std::array<std::string, 9> files = {
            "Camp.HC", "WaterRock.HC", "Battle.HC", "Quail.HC",
            "Mountain.HC", "Clouds.HC", "GodTalking.HC", "AfterEgypt.HC", "HorebA.HC",
        };
        for (const auto &file : files) loadDoc(*root / file, file);
        loadDoc(*root / "AESplash.DD", "AESplash.DD");

        const fs::path comics = *root / "Comics";
        const std::array<std::string, 7> comicFiles = {
            "Moses01.DD", "Moses02.DD", "Moses04.DD", "Moses05.DD",
            "Moses06.DD", "Moses07.DD", "Moses08.DD",
        };
        for (const auto &file : comicFiles) loadDoc(comics / file, "Comics/" + file);
    }

    void clear()
    {
        for (auto &[_, sprite] : sprites_) {
            if (sprite.texture) SDL_DestroyTexture(sprite.texture);
        }
        sprites_.clear();
    }

    bool draw(const std::string &file, int bi, float x, float y, float scale = 1.0f,
              uint8_t alpha = 255, bool flip = false) const
    {
        const auto it = sprites_.find(key(file, bi));
        if (it == sprites_.end() || !it->second.texture) return false;
        const TempleSprite &sprite = it->second;
        SDL_FRect dst{
            x + sprite.originX * scale,
            y + sprite.originY * scale,
            sprite.width * scale,
            sprite.height * scale,
        };
        if (alpha != 255) SDL_SetTextureAlphaMod(sprite.texture, alpha);
        if (flip) {
            SDL_RenderTextureRotated(renderer_, sprite.texture, nullptr, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
        } else {
            SDL_RenderTexture(renderer_, sprite.texture, nullptr, &dst);
        }
        if (alpha != 255) SDL_SetTextureAlphaMod(sprite.texture, 255);
        return true;
    }

    bool has(const std::string &file, int bi) const
    {
        return sprites_.contains(key(file, bi));
    }

    bool drawCentered(const std::string &file, int bi, float cx, float cy,
                      float maxW, float maxH) const
    {
        const auto it = sprites_.find(key(file, bi));
        if (it == sprites_.end() || !it->second.texture) return false;
        const TempleSprite &sprite = it->second;
        const float scale = std::min(maxW / std::max(1, sprite.width),
                                     maxH / std::max(1, sprite.height));
        const float anchorX = cx - (sprite.originX + sprite.width * 0.5f) * scale;
        const float anchorY = cy - (sprite.originY + sprite.height * 0.5f) * scale;
        return draw(file, bi, anchorX, anchorY, scale);
    }

private:
    struct Bounds {
        int minX = 1000000;
        int minY = 1000000;
        int maxX = -1000000;
        int maxY = -1000000;
    };

    SDL_Renderer *renderer_ = nullptr;
    std::map<std::string, TempleSprite> sprites_;

    static std::string key(const std::string &file, int bi)
    {
        return file + ":" + std::to_string(bi);
    }

    static std::optional<fs::path> findAfterEgyptDir()
    {
        return findAfterEgyptAssetsDir();
    }

    static uint32_t readU32(const std::vector<uint8_t> &bytes, size_t off)
    {
        if (off + 4 > bytes.size()) return 0;
        return static_cast<uint32_t>(bytes[off]) |
               static_cast<uint32_t>(bytes[off + 1]) << 8 |
               static_cast<uint32_t>(bytes[off + 2]) << 16 |
               static_cast<uint32_t>(bytes[off + 3]) << 24;
    }

    static int32_t readI32(const std::vector<uint8_t> &bytes, size_t off)
    {
        return static_cast<int32_t>(readU32(bytes, off));
    }

    static size_t elemSize(const std::vector<uint8_t> &data, size_t p)
    {
        if (p >= data.size()) return 0;
        const uint8_t type = data[p] & 0x7F;
        switch (type) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 3;
        case 3: return 5;
        case 4:
        case 10:
        case 12:
        case 26: return 17;
        case 5:
        case 6: return 1;
        case 7:
        case 8:
        case 21:
        case 22: return 9;
        case 14: return 13;
        case 13:
        case 15: return 25;
        case 16: return 29;
        case 11: {
            const int32_t n = readI32(data, p + 1);
            return n < 0 ? 0 : 5 + static_cast<size_t>(n) * 8;
        }
        case 17:
        case 18:
        case 19:
        case 20: {
            const int32_t n = readI32(data, p + 1);
            return n < 0 ? 0 : 5 + static_cast<size_t>(n) * 12;
        }
        case 23: {
            const int32_t w = readI32(data, p + 9);
            const int32_t h = readI32(data, p + 13);
            if (w < 0 || h < 0) return 0;
            return 17 + static_cast<size_t>((w + 7) & ~7) * static_cast<size_t>(h);
        }
        case 27:
        case 28:
        case 29: {
            size_t e = p + 9;
            while (e < data.size() && data[e] != 0) ++e;
            return e < data.size() ? e - p + 1 : 0;
        }
        default:
            return 0;
        }
    }

    static void addPoint(Bounds &bounds, int x, int y, int pad = 0)
    {
        bounds.minX = std::min(bounds.minX, x - pad);
        bounds.minY = std::min(bounds.minY, y - pad);
        bounds.maxX = std::max(bounds.maxX, x + pad);
        bounds.maxY = std::max(bounds.maxY, y + pad);
    }

    static void addRect(Bounds &bounds, int x1, int y1, int x2, int y2, int pad = 0)
    {
        addPoint(bounds, std::min(x1, x2), std::min(y1, y2), pad);
        addPoint(bounds, std::max(x1, x2), std::max(y1, y2), pad);
    }

    static std::array<Color, 16> palette()
    {
        return {{
            {0, 0, 0},       {0, 0, 170},     {0, 170, 0},     {0, 170, 170},
            {170, 0, 0},     {170, 0, 170},   {170, 85, 0},    {170, 170, 170},
            {85, 85, 85},    {85, 85, 255},   {85, 255, 85},   {85, 255, 255},
            {255, 85, 85},   {255, 85, 255},  {255, 255, 85},  {255, 255, 255},
        }};
    }

    void loadDoc(const fs::path &path, const std::string &name)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return;
        const std::vector<char> raw((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
        const std::vector<uint8_t> bytes(raw.begin(), raw.end());
        const auto nul = std::find(bytes.begin(), bytes.end(), 0);
        if (nul == bytes.end()) return;

        size_t p = static_cast<size_t>(std::distance(bytes.begin(), nul)) + 1;
        while (p + 16 <= bytes.size()) {
            const uint32_t num = readU32(bytes, p);
            const uint32_t size = readU32(bytes, p + 8);
            if (size == 0 || p + 16 + size > bytes.size()) break;

            std::vector<uint8_t> data(bytes.begin() + static_cast<std::ptrdiff_t>(p + 16),
                                      bytes.begin() + static_cast<std::ptrdiff_t>(p + 16 + size));
            if (SpriteImage image = renderSprite(data); image.width > 0 && image.height > 0) {
                SDL_Texture *texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                                         SDL_TEXTUREACCESS_STATIC,
                                                         image.width, image.height);
                if (texture) {
                    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
                    SDL_UpdateTexture(texture, nullptr, image.pixels.data(), image.width * 4);
                    sprites_[key(name, static_cast<int>(num))] = {
                        image.width, image.height, image.originX, image.originY, texture};
                }
            }
            p += 16 + size;
        }
    }

    static SpriteImage renderSprite(const std::vector<uint8_t> &data)
    {
        Bounds bounds;
        int shiftX = 0;
        int shiftY = 0;
        int thick = 1;

        for (size_t p = 0; p < data.size();) {
            const uint8_t type = data[p] & 0x7F;
            const size_t size = elemSize(data, p);
            if (size == 0 || p + size > data.size()) break;
            if (type == 0) break;

            switch (type) {
            case 3:
                thick = std::max(1, readI32(data, p + 1));
                break;
            case 7:
                shiftX += readI32(data, p + 1);
                shiftY += readI32(data, p + 5);
                break;
            case 8:
            case 21:
            case 22:
                addPoint(bounds, shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5), thick);
                break;
            case 10:
            case 12:
            case 26:
                addRect(bounds, shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5),
                        shiftX + readI32(data, p + 9), shiftY + readI32(data, p + 13), thick);
                break;
            case 11: {
                const int32_t n = readI32(data, p + 1);
                for (int i = 0; i < n; ++i) {
                    const size_t off = p + 5 + static_cast<size_t>(i) * 8;
                    addPoint(bounds, shiftX + readI32(data, off), shiftY + readI32(data, off + 4), thick);
                }
                break;
            }
            case 14: {
                const int x = shiftX + readI32(data, p + 1);
                const int y = shiftY + readI32(data, p + 5);
                const int r = readI32(data, p + 9);
                addRect(bounds, x - r, y - r, x + r, y + r, thick);
                break;
            }
            case 17:
            case 18:
            case 19:
            case 20: {
                const int32_t n = readI32(data, p + 1);
                for (int i = 0; i < n; ++i) {
                    const size_t off = p + 5 + static_cast<size_t>(i) * 12;
                    addPoint(bounds, shiftX + readI32(data, off), shiftY + readI32(data, off + 4), thick);
                }
                break;
            }
            case 23: {
                const int x = shiftX + readI32(data, p + 1);
                const int y = shiftY + readI32(data, p + 5);
                const int w = readI32(data, p + 9);
                const int h = readI32(data, p + 13);
                addRect(bounds, x, y, x + w - 1, y + h - 1);
                break;
            }
            case 27:
            case 28:
            case 29:
                addRect(bounds, shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5),
                        shiftX + readI32(data, p + 1) + static_cast<int>(size) * 8,
                        shiftY + readI32(data, p + 5) + 16);
                break;
            default:
                break;
            }
            p += size;
        }

        if (bounds.maxX < bounds.minX || bounds.maxY < bounds.minY) return {};

        SpriteImage image;
        image.originX = bounds.minX;
        image.originY = bounds.minY;
        image.width = bounds.maxX - bounds.minX + 1;
        image.height = bounds.maxY - bounds.minY + 1;
        std::vector<uint8_t> canvas(static_cast<size_t>(image.width) * image.height, 0xFF);

        auto plot = [&](int x, int y, uint8_t color, int pen = 1) {
            if (color == 0xFF) return;
            const int r = std::max(0, pen / 2);
            for (int yy = y - r; yy <= y + r; ++yy) {
                for (int xx = x - r; xx <= x + r; ++xx) {
                    const int cx = xx - image.originX;
                    const int cy = yy - image.originY;
                    if (cx >= 0 && cy >= 0 && cx < image.width && cy < image.height) {
                        canvas[static_cast<size_t>(cy) * image.width + cx] = color;
                    }
                }
            }
        };

        auto drawLine = [&](int x1, int y1, int x2, int y2, uint8_t color, int pen) {
            int dx = std::abs(x2 - x1);
            int sx = x1 < x2 ? 1 : -1;
            int dy = -std::abs(y2 - y1);
            int sy = y1 < y2 ? 1 : -1;
            int err = dx + dy;
            while (true) {
                plot(x1, y1, color, pen);
                if (x1 == x2 && y1 == y2) break;
                const int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x1 += sx; }
                if (e2 <= dx) { err += dx; y1 += sy; }
            }
        };

        auto drawCircle = [&](int cx, int cy, int r, uint8_t color, int pen) {
            constexpr int steps = 96;
            int px = cx + r;
            int py = cy;
            for (int i = 1; i <= steps; ++i) {
                const double a = 2.0 * kPi * i / steps;
                const int x = cx + static_cast<int>(std::lround(std::cos(a) * r));
                const int y = cy + static_cast<int>(std::lround(std::sin(a) * r));
                drawLine(px, py, x, y, color, pen);
                px = x;
                py = y;
            }
        };

        shiftX = 0;
        shiftY = 0;
        thick = 1;
        uint8_t color = 15;
        for (size_t p = 0; p < data.size();) {
            const uint8_t type = data[p] & 0x7F;
            const size_t size = elemSize(data, p);
            if (size == 0 || p + size > data.size()) break;
            if (type == 0) break;

            switch (type) {
            case 1:
                color = data[p + 1];
                break;
            case 2:
                color = static_cast<uint8_t>(data[p + 1] & 0x0F);
                break;
            case 3:
                thick = std::max(1, readI32(data, p + 1));
                break;
            case 7:
                shiftX += readI32(data, p + 1);
                shiftY += readI32(data, p + 5);
                break;
            case 8:
                plot(shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5), color, thick);
                break;
            case 10:
            case 26:
                drawLine(shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5),
                         shiftX + readI32(data, p + 9), shiftY + readI32(data, p + 13), color, thick);
                break;
            case 12: {
                const int x1 = shiftX + readI32(data, p + 1);
                const int y1 = shiftY + readI32(data, p + 5);
                const int x2 = shiftX + readI32(data, p + 9);
                const int y2 = shiftY + readI32(data, p + 13);
                for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
                    for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
                        plot(x, y, color);
                break;
            }
            case 11: {
                const int32_t n = readI32(data, p + 1);
                if (n > 1) {
                    int px = shiftX + readI32(data, p + 5);
                    int py = shiftY + readI32(data, p + 9);
                    for (int i = 1; i < n; ++i) {
                        const size_t off = p + 5 + static_cast<size_t>(i) * 8;
                        const int x = shiftX + readI32(data, off);
                        const int y = shiftY + readI32(data, off + 4);
                        drawLine(px, py, x, y, color, thick);
                        px = x;
                        py = y;
                    }
                }
                break;
            }
            case 14:
                drawCircle(shiftX + readI32(data, p + 1), shiftY + readI32(data, p + 5),
                           readI32(data, p + 9), color, thick);
                break;
            case 17:
            case 18:
            case 19:
            case 20: {
                const int32_t n = readI32(data, p + 1);
                if (n > 1) {
                    int firstX = shiftX + readI32(data, p + 5);
                    int firstY = shiftY + readI32(data, p + 9);
                    int px = firstX;
                    int py = firstY;
                    for (int i = 1; i < n; ++i) {
                        const size_t off = p + 5 + static_cast<size_t>(i) * 12;
                        const int x = shiftX + readI32(data, off);
                        const int y = shiftY + readI32(data, off + 4);
                        drawLine(px, py, x, y, color, thick);
                        px = x;
                        py = y;
                    }
                    if (type == 18 || type == 20) {
                        drawLine(px, py, firstX, firstY, color, thick);
                    }
                }
                break;
            }
            case 23: {
                const int x = shiftX + readI32(data, p + 1);
                const int y = shiftY + readI32(data, p + 5);
                const int w = readI32(data, p + 9);
                const int h = readI32(data, p + 13);
                const int wi = (w + 7) & ~7;
                const size_t body = p + 17;
                for (int yy = 0; yy < h; ++yy) {
                    for (int xx = 0; xx < w; ++xx) {
                        plot(x + xx, y + yy, data[body + static_cast<size_t>(yy) * wi + xx]);
                    }
                }
                break;
            }
            default:
                break;
            }
            p += size;
        }

        const auto pal = palette();
        image.pixels.resize(canvas.size());
        for (size_t i = 0; i < canvas.size(); ++i) {
            const uint8_t c = canvas[i];
            if (c == 0xFF) {
                image.pixels[i] = 0x00000000;
            } else {
                const Color rgb = pal[c & 0x0F];
                image.pixels[i] = 0xFF000000u |
                                  static_cast<uint32_t>(rgb.r) << 16 |
                                  static_cast<uint32_t>(rgb.g) << 8 |
                                  static_cast<uint32_t>(rgb.b);
            }
        }
        return image;
    }
};

class AfterEgyptApp {
public:
    AfterEgyptApp()
    {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(SDL_GetError());
        }
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        if (!TTF_Init()) {
            throw std::runtime_error(SDL_GetError());
        }
        if (!SDL_CreateWindowAndRenderer("AfterEgypt SDL3", kWindowW, kWindowH,
                                         kWindowFlags, &window_, &renderer_)) {
            throw std::runtime_error(SDL_GetError());
        }
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

        const fs::path font = findFont();
        font_ = TTF_OpenFont(font.string().c_str(), 17.0f);
        smallFont_ = TTF_OpenFont(font.string().c_str(), 14.0f);
        titleFont_ = TTF_OpenFont(font.string().c_str(), 32.0f);
        if (!font_ || !smallFont_ || !titleFont_) {
            throw std::runtime_error(SDL_GetError());
        }

        sprites_.load(renderer_);
        textAssets_.load();
        audio_.init();
        resetAll();
        introStart_ = nowSeconds();
        showIntro_ = true;
    }

    ~AfterEgyptApp()
    {
        audio_.shutdown();
        sprites_.clear();
        if (titleFont_) TTF_CloseFont(titleFont_);
        if (smallFont_) TTF_CloseFont(smallFont_);
        if (font_) TTF_CloseFont(font_);
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        TTF_Quit();
        SDL_Quit();
    }

    int run(int maxFrames = 0)
    {
        bool running = true;
        int frames = 0;
        double last = nowSeconds();

        while (running) {
            const double t = nowSeconds();
            const double dt = clamp(t - last, 0.0, 0.05);
            last = t;

            handleEvents(running);
            if (quitRequested_) running = false;
            update(dt, t);
            pumpAudio(dt);
            render(t);

            if (maxFrames > 0 && ++frames >= maxFrames) {
                running = false;
            }
            SDL_Delay(10);
        }

        return 0;
    }

    int smokeScenes()
    {
        if (!textAssets_.hasBible()) {
            std::cerr << "after-egypt-sdl3: Bible.TXT asset not found\n";
            return 2;
        }
        const std::optional<fs::path> afterEgyptRoot = findAfterEgyptAssetsDir();
        if (!afterEgyptRoot) {
            std::cerr << "after-egypt-sdl3: AfterEgypt asset directory not found\n";
            return 2;
        }
        const std::array<fs::path, 5> required = {
            "Camp.HC",
            "WaterRock.HC",
            "Battle.HC",
            "Quail.HC",
            "HorebA.HC",
        };
        for (const auto &rel : required) {
            if (!fs::exists(*afterEgyptRoot / rel)) {
                std::cerr << "after-egypt-sdl3: missing asset " << (*afterEgyptRoot / rel) << '\n';
                return 2;
            }
        }
        for (const auto &file : comicFiles_) {
            if (!sprites_.has(file, 1)) {
                std::cerr << "after-egypt-sdl3: missing comic sprite " << file << '\n';
                return 2;
            }
        }

        showIntro_ = false;
        const auto frame = [&] {
            const double t = nowSeconds();
            update(1.0 / 60.0, t);
            pumpAudio(1.0 / 60.0);
            render(t);
            SDL_Delay(5);
        };

        flow_ = Flow::CampWatch;
        frame();
        flow_ = Flow::Menu;
        frame();

        const std::array<Mode, 8> modes = {
            Mode::Clouds, Mode::Court, Mode::Map, Mode::WaterRock,
            Mode::Battle, Mode::Quail, Mode::Comics, Mode::Help,
        };
        for (Mode mode : modes) {
            startScene(mode);
            frame();
            if (mode == Mode::Quail) {
                beginQuailAnimation();
                frame();
            }
            if (mode == Mode::Comics) {
                comicPicker_ = false;
                frame();
            }
        }

        startScene(Mode::God);
        godStage_ = GodStage::Climb;
        frame();
        godStage_ = GodStage::Horeb;
        frame();
        godStage_ = GodStage::Talking;
        godTextLines_ = textAssets_.randomGodText(rng_);
        frame();
        return 0;
    }

private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    TTF_Font *font_ = nullptr;
    TTF_Font *smallFont_ = nullptr;
    TTF_Font *titleFont_ = nullptr;
    TempleSprites sprites_;
    TextAssets textAssets_;
    AudioEngine audio_;

    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
    Flow flow_ = Flow::CampWatch;
    Mode mode_ = Mode::Camp;
    int day_ = 0;
    int people_ = 100;

    std::vector<CampObj> camp_;
    std::vector<Cloud> clouds_;
    std::array<CloudPen, kCloudPens> cloudPens_;
    std::vector<SDL_FPoint> mapPath_;
    std::vector<Quail> quail_;
    std::vector<HorebObj> horebObjs_;
    std::vector<std::string> comicFiles_;
    std::vector<std::string> godTextLines_;
    std::string courtPrompt_;
    bool showIntro_ = true;
    bool quitRequested_ = false;
    double introStart_ = 0.0;
    double sceneStart_ = 0.0;
    double campStart_ = 0.0;
    double campStepAccum_ = 0.0;
    int campCalfCycle_ = -1;
    bool campShowCalf_ = false;
    double cloudStepAccum_ = 0.0;
    double mountainStart_ = 0.0;
    GodStage godStage_ = GodStage::Climb;
    double godStageStart_ = 0.0;
    double horebAngle_ = 0.0;
    double horebX_ = 0.0;
    double horebZ_ = 0.0;
    bool horebFound_ = false;
    double horebFoundTime_ = 0.0;
    double mapWorldX_ = 0.0;
    double mapWorldY_ = 0.0;
    double mapA1_ = 0.0;
    double mapA2_ = 0.0;
    double mapA2Total_ = 0.0;
    double mapStepDue_ = 0.0;
    bool mapLastLeft_ = false;
    double waterDownTime_ = -1.0;
    double waterUpTime_ = -1.0;
    double waterAutoReleaseAt_ = -1.0;
    bool waterDownStroke_ = false;
    double battleT0_ = 0.0;
    double battleLast_ = 0.0;
    double battleTT_ = 0.0;
    double battleShift_ = 0.0;
    bool battleHeld_ = false;
    bool battleMouseHeld_ = false;
    bool quailReading_ = true;
    bool comicPicker_ = true;
    int docScroll_ = 0;
    int comicIndex_ = 0;

    double random()
    {
        return unit_(rng_);
    }

    double range(double lo, double hi)
    {
        return lo + (hi - lo) * random();
    }

    int irange(int lo, int hi)
    {
        return lo + static_cast<int>(random() * (hi - lo + 1));
    }

    void resetAll()
    {
        day_ = 0;
        people_ = 100;
        initClouds();
        initMap();
        initQuail();
        initHoreb();
        initComics();
        generateCourt();
        resetWater();
        resetBattle();
        startTurn();
        mountainStart_ = nowSeconds();
        godStageStart_ = mountainStart_;
        godStage_ = GodStage::Climb;
    }

    void startTurn()
    {
        ++day_;
        const double growth = 1.0 + 0.01 * static_cast<double>(irange(-30, 69));
        people_ = std::min(static_cast<int>(people_ * growth), 1024);
        initCamp();
        flow_ = Flow::CampWatch;
        mode_ = Mode::Camp;
        campStart_ = nowSeconds();
        campStepAccum_ = 0.0;
        campCalfCycle_ = -1;
        campShowCalf_ = false;
        docScroll_ = 0;
    }

    void initCamp()
    {
        camp_.clear();
        const int tents = (people_ + 39) / 40;
        const int personCount = std::min(people_, 1024);

        for (int i = 0; i < personCount; ++i) {
            CampObj obj;
            obj.x = range(-kCampWidth / 2.0, kCampWidth / 2.0);
            obj.z = -range(0.0, kCampWidth);
            obj.tent = false;
            camp_.push_back(obj);
        }

        for (int i = 0; i < tents; ++i) {
            CampObj obj;
            obj.x = range(-kCampWidth / 2.0, kCampWidth / 2.0);
            obj.z = -range(0.0, kCampWidth);
            obj.tent = true;
            camp_.push_back(obj);
        }
    }

    double cloudSkyHeight(int h) const
    {
        const double rowH = clamp(h / 36.0, 12.0, 16.0);
        return std::min(h * 0.70, rowH * 30.0);
    }

    void initClouds()
    {
        clouds_.clear();
        clouds_.reserve(kCloudsNum);
        int w = 0;
        int h = 0;
        if (window_) SDL_GetWindowSize(window_, &w, &h);
        if (w <= 0) w = kWindowW;
        if (h <= 0) h = kWindowH;
        const double skyH = cloudSkyHeight(h);
        for (int i = 0; i < kCloudsNum; ++i) {
            Cloud cloud;
            cloud.x = range(w * 0.25, w * 0.75);
            cloud.y = range(skyH * 0.25, skyH * 0.75);
            cloud.dx = range(-25.0, 25.0);
            cloud.dy = range(-25.0, 25.0);
            cloud.color = static_cast<uint16_t>(irange(0, 65535));
            for (CloudPoint &point : cloud.points) {
                int sx = 0;
                int sy = 0;
                for (int j = 0; j < kCloudSamples; ++j) {
                    sx += irange(-32768, 32767);
                    sy += irange(-32768, 32767);
                }
                point.x = sx * 100 / 32767 / kCloudSamples;
                point.y = sy * 50 / 32767 / kCloudSamples;
                point.shade = static_cast<uint16_t>(irange(0, 65535));
            }
            clouds_.push_back(cloud);
        }
        for (CloudPen &pen : cloudPens_) {
            for (SDL_Point &point : pen.points) {
                point.x = irange(0, kCloudPenSize - 1);
                point.y = irange(0, kCloudPenSize - 1);
            }
        }
        cloudStepAccum_ = 0.0;
    }

    void initMap()
    {
        mapPath_.clear();
        mapPath_.push_back({0.0f, 0.0f});
        mapWorldX_ = 0.0;
        mapWorldY_ = 0.0;
        mapA1_ = (0.05 + 0.02) / 2.0;
        mapA2_ = (0.30 + 0.15) / 2.0;
        mapA2Total_ = mapA2_;
        mapLastLeft_ = false;
        mapStepDue_ = 0.0;
    }

    void initQuail()
    {
        quail_.clear();
        int w = 0;
        int h = 0;
        if (window_) SDL_GetWindowSize(window_, &w, &h);
        if (w <= 0) w = kWindowW;
        if (h <= 0) h = kWindowH;
        const double skyH = 0.6 * h;
        for (int i = 0; i < 128; ++i) {
            Quail q;
            q.x = range(-0.2 * w, static_cast<double>(w));
            q.y = range(0.0, skyH);
            q.dx = range(10.0, 60.0);
            q.dy = range(-10.0, 10.0);
            q.phase = range(0.0, 1.0);
            q.dead = false;
            quail_.push_back(q);
        }
    }

    void initHoreb()
    {
        horebObjs_.clear();
        horebObjs_.reserve(256 + 4096);
        horebAngle_ = 0.0;
        horebX_ = 0.0;
        horebZ_ = 0.0;
        horebFound_ = false;
        horebFoundTime_ = 0.0;

        auto coord = [&] { return range(-4096.0, 4095.0); };
        auto weightedType = [&] {
            const std::array<std::pair<int, int>, 8> weights = {{
                {1, 30}, {2, 30}, {3, 15}, {4, 30},
                {5, 30}, {6, 1},  {7, 1},  {8, 1},
            }};
            int total = 0;
            for (const auto &[_, weight] : weights) total += weight;
            int pick = irange(0, total - 1);
            for (const auto &[type, weight] : weights) {
                pick -= weight;
                if (pick < 0) return type;
            }
            return 1;
        };
        auto makeObj = [&](int type, int seed) {
            HorebObj obj;
            obj.x = coord();
            obj.y = 0.0;
            obj.z = coord();
            obj.phase = 2.0 * kPi * random();
            obj.bi = type;
            obj.seed = seed;
            obj.mirror = irange(0, 1) == 0;
            return obj;
        };

        horebObjs_.push_back(makeObj(1, 0));
        for (int i = 1; i < 256; ++i) {
            horebObjs_.push_back(makeObj(weightedType(), i));
        }
        for (int i = 0; i < 4096; ++i) {
            horebObjs_.push_back(makeObj(0, 10000 + i));
        }
    }

    void initComics()
    {
        comicFiles_ = {
            "Comics/Moses01.DD",
            "Comics/Moses02.DD",
            "Comics/Moses04.DD",
            "Comics/Moses05.DD",
            "Comics/Moses06.DD",
            "Comics/Moses07.DD",
            "Comics/Moses08.DD",
        };
    }

    void breakCamp()
    {
        startTurn();
    }

    void restartCurrent()
    {
        if (flow_ == Flow::CampWatch) {
            initCamp();
            campStart_ = nowSeconds();
            return;
        }
        if (flow_ == Flow::Menu) return;
        switch (mode_) {
        case Mode::Camp: startTurn(); break;
        case Mode::God: startGod(); break;
        case Mode::Clouds: initClouds(); break;
        case Mode::Court: generateCourt(); break;
        case Mode::Map: initMap(); break;
        case Mode::WaterRock: resetWater(); break;
        case Mode::Battle: resetBattle(); break;
        case Mode::Quail: initQuail(); quailReading_ = true; docScroll_ = 0; break;
        case Mode::Comics: comicIndex_ = 0; comicPicker_ = true; break;
        case Mode::Help: docScroll_ = 0; break;
        }
    }

    void startScene(Mode mode)
    {
        flow_ = Flow::Scene;
        mode_ = mode;
        sceneStart_ = nowSeconds();
        docScroll_ = 0;
        if (mode_ == Mode::God) startGod();
        if (mode_ == Mode::Clouds) initClouds();
        if (mode_ == Mode::Court) generateCourt();
        if (mode_ == Mode::Map) initMap();
        if (mode_ == Mode::WaterRock) resetWater();
        if (mode_ == Mode::Battle) resetBattle();
        if (mode_ == Mode::Quail) {
            initQuail();
            quailReading_ = true;
        }
        if (mode_ == Mode::Comics) {
            comicPicker_ = true;
            comicIndex_ = 0;
        }
    }

    void finishScene()
    {
        startTurn();
    }

    void startGod()
    {
        mountainStart_ = nowSeconds();
        godStageStart_ = mountainStart_;
        godStage_ = GodStage::Climb;
        godTextLines_.clear();
        initHoreb();
    }

    void resetWater()
    {
        waterDownTime_ = -1.0;
        waterUpTime_ = -1.0;
        waterAutoReleaseAt_ = -1.0;
        waterDownStroke_ = false;
    }

    void resetBattle()
    {
        battleT0_ = nowSeconds();
        battleLast_ = 0.0;
        battleTT_ = 0.0;
        battleShift_ = 0.0;
        battleHeld_ = false;
        battleMouseHeld_ = false;
    }

    void perform(Action action)
    {
        switch (action) {
        case Action::BreakCamp: breakCamp(); break;
        case Action::God: startScene(Mode::God); break;
        case Action::Clouds: startScene(Mode::Clouds); break;
        case Action::Court: startScene(Mode::Court); break;
        case Action::Map: startScene(Mode::Map); break;
        case Action::WaterRock: startScene(Mode::WaterRock); break;
        case Action::Battle: startScene(Mode::Battle); break;
        case Action::Quail: startScene(Mode::Quail); break;
        case Action::Comics: startScene(Mode::Comics); break;
        case Action::Help: startScene(Mode::Help); break;
        case Action::Quit: quitRequested_ = true; break;
        }
    }

    void handleEvents(bool &running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (showIntro_) {
                    showIntro_ = false;
                    continue;
                }
                onMouseDown(event.button.x, event.button.y);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (flow_ == Flow::Scene && mode_ == Mode::Battle) {
                    battleMouseHeld_ = false;
                    if (!spaceIsDown()) releaseBattle();
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (showIntro_) {
                    if (!ignoresAnyKey(event.key.scancode, event.key.mod)) {
                        showIntro_ = false;
                    }
                    continue;
                }
                onKeyDown(event.key.key, event.key.scancode, event.key.mod, event.key.repeat, running);
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.scancode == SDL_SCANCODE_SPACE) {
                    if (flow_ == Flow::Scene && mode_ == Mode::Battle && !battleMouseHeld_) releaseBattle();
                    if (flow_ == Flow::Scene && mode_ == Mode::WaterRock) releaseWaterStrike();
                }
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                onWheel(event.wheel.y);
            }
        }
    }

    void onMouseDown(float x, float y)
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(window_, &w, &h);

        if (flow_ == Flow::CampWatch) {
            openMenu();
            return;
        }

        if (flow_ == Flow::Menu) {
            for (const auto &button : layoutButtons(w, h)) {
                if (pointIn(button.rect, x, y)) {
                    perform(button.action);
                    return;
                }
            }
            return;
        }

        if (mode_ == Mode::WaterRock) {
            quickWaterStrike();
        } else if (mode_ == Mode::Battle) {
            battleMouseHeld_ = true;
            holdBattle();
        } else if (mode_ == Mode::Court) {
            const int choice = courtChoiceAt(x, y, w, h);
            if (choice >= 0) judge(choice);
        } else if (mode_ == Mode::Comics) {
            if (comicPicker_) {
                const int choice = comicChoiceAt(x, y, w, h);
                if (choice >= 0) {
                    comicIndex_ = choice;
                    comicPicker_ = false;
                }
            } else {
                comicPicker_ = true;
            }
        } else if (mode_ == Mode::Quail && quailReading_) {
            beginQuailAnimation();
        } else if (mode_ == Mode::Map || mode_ == Mode::Clouds) {
            finishScene();
        }
    }

    void onWheel(float y)
    {
        if (flow_ != Flow::Scene) return;
        if (mode_ != Mode::Help && !(mode_ == Mode::Quail && quailReading_)) return;
        docScroll_ = std::max(0, docScroll_ - static_cast<int>(std::lround(y * 54.0f)));
    }

    void onKeyDown(SDL_Keycode key, SDL_Scancode scancode, SDL_Keymod mod, bool repeat, bool &running)
    {
        (void)running;
        const bool ignoreAnyKey = ignoresAnyKey(scancode, mod);
        if (key == SDLK_M) {
            audio_.toggleMuted();
            return;
        }

        if (flow_ == Flow::CampWatch) {
            if (!ignoreAnyKey) openMenu();
            return;
        }

        if (flow_ == Flow::Menu) {
            if (ignoreAnyKey) return;
            if (key == SDLK_ESCAPE) {
                flow_ = Flow::CampWatch;
                return;
            }
            if (key == SDLK_Q) {
                quitRequested_ = true;
                return;
            }
            const int actionIndex = menuShortcutIndex(key);
            if (actionIndex >= 0) {
                const auto buttons = layoutButtons(1000, 760);
                if (actionIndex < static_cast<int>(buttons.size())) perform(buttons[static_cast<size_t>(actionIndex)].action);
            }
            return;
        }

        if (key == SDLK_ESCAPE || key == SDLK_Q) {
            if (mode_ == Mode::Comics && !comicPicker_) {
                comicPicker_ = true;
                return;
            }
            finishScene();
            return;
        }

        if ((mode_ == Mode::Help || (mode_ == Mode::Quail && quailReading_)) &&
            handleDocKey(scancode)) {
            return;
        }

        if (mode_ == Mode::Quail && quailReading_ &&
            (key == SDLK_RETURN || key == SDLK_SPACE)) {
            beginQuailAnimation();
            return;
        }

        if (key == SDLK_RETURN && mode_ == Mode::God && godStage_ == GodStage::Horeb) {
            initHoreb();
            godStageStart_ = nowSeconds();
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_R) {
            restartCurrent();
            return;
        }

        if (mode_ == Mode::God && godStage_ == GodStage::Horeb) {
            if (scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_DOWN ||
                scancode == SDL_SCANCODE_LEFT || scancode == SDL_SCANCODE_RIGHT) {
                moveHoreb(scancode);
                return;
            }
        }

        if (key == SDLK_SPACE) {
            if (mode_ == Mode::WaterRock && !repeat) beginWaterStrike();
            else if (mode_ == Mode::Battle) holdBattle();
            else if (!ignoreAnyKey && (mode_ == Mode::Map || mode_ == Mode::Clouds ||
                                       (mode_ == Mode::Quail && !quailReading_))) {
                finishScene();
            }
            return;
        }

        if (ignoreAnyKey) return;

        if (mode_ == Mode::Court) {
            if (key == SDLK_1) judge(0);
            if (key == SDLK_2) judge(1);
            if (key == SDLK_3) judge(2);
        } else if (mode_ == Mode::Comics) {
            if (comicPicker_) {
                if (scancode == SDL_SCANCODE_LEFT) previousComic();
                if (scancode == SDL_SCANCODE_RIGHT) nextComic();
                if (key == SDLK_RETURN || key == SDLK_SPACE) comicPicker_ = false;
            } else {
                if (scancode == SDL_SCANCODE_LEFT) previousComic();
                if (scancode == SDL_SCANCODE_RIGHT) nextComic();
            }
        } else if (mode_ == Mode::Map || mode_ == Mode::Clouds ||
                   (mode_ == Mode::Quail && !quailReading_)) {
            finishScene();
        }
    }

    void update(double dt, double t)
    {
        if (showIntro_ && t - introStart_ >= 6.6) {
            showIntro_ = false;
        }
        if (waterAutoReleaseAt_ > 0.0 && t >= waterAutoReleaseAt_) {
            releaseWaterStrike();
            waterAutoReleaseAt_ = -1.0;
        }
        if (flow_ == Flow::CampWatch) updateCamp(dt, t);
        if (flow_ != Flow::Scene) return;
        updateGod(dt, t);
        if (mode_ == Mode::Clouds) updateClouds(dt);
        if (mode_ == Mode::Map) updateMap(dt, t);
        if (mode_ == Mode::Battle) updateBattle(dt, t);
        if (mode_ == Mode::Quail && !quailReading_) updateQuail(dt, t);
    }

    void pumpAudio(double dt)
    {
        if (showIntro_ || flow_ == Flow::CampWatch) {
            audio_.pump(Mode::Camp, dt);
        } else if (flow_ == Flow::Scene && mode_ == Mode::Clouds) {
            audio_.pump(Mode::Clouds, dt);
        } else {
            audio_.silence();
        }
    }

    void updateGod(double dt, double t)
    {
        if (mode_ != Mode::God) return;
        if (godStage_ == GodStage::Climb && t - mountainStart_ >= 7.5) {
            godStage_ = GodStage::Horeb;
            godStageStart_ = t;
        }
        if (godStage_ == GodStage::Horeb) updateHorebAnimals(dt, t);
        if (godStage_ == GodStage::Horeb && horebFound_ && t - horebFoundTime_ >= 0.20) {
            advanceGodStage();
        }
    }

    void updateHorebAnimals(double dt, double t)
    {
        for (HorebObj &obj : horebObjs_) {
            if (obj.bi == 6 || obj.bi == 7 || obj.bi == 8) {
                obj.x += obj.dx * dt;
                obj.z += obj.dz * dt;
                obj.dx = 250.0 * std::cos(0.5 * t + obj.phase);
                obj.dz = 250.0 * std::sin(0.5 * t + obj.phase);
                const double screenDx = obj.dx * std::cos(horebAngle_) - obj.dz * std::sin(horebAngle_);
                obj.mirror = screenDx < 0.0;
            }
        }
    }

    void updateCamp(double dt, double t)
    {
        constexpr double step = 0.020;
        campStepAccum_ = std::min(campStepAccum_ + dt, step * 8.0);
        while (campStepAccum_ >= step) {
            const double localT = t - campStart_ - campStepAccum_;
            const bool gather = std::fmod(localT, 10.0) >= 5.0;
            const int cycle = static_cast<int>(std::floor(localT / 10.0));
            if (cycle != campCalfCycle_) {
                campCalfCycle_ = cycle;
                campShowCalf_ = gather && random() < 0.5;
            }

            constexpr double speedMax = 2.0;
            const int numObjs = std::max(1, static_cast<int>(camp_.size()));
            for (size_t i = 0; i < camp_.size(); ++i) {
                CampObj &obj = camp_[i];
                if (obj.tent) continue;
                if (gather) {
                    const double a = (1.0 + 1.0 / 40.0) * static_cast<double>(i) *
                                     2.0 * kPi / static_cast<double>(numObjs);
                    const double tx = kCampWidth / 4.0 * std::cos(a);
                    const double tz = kCampWidth / 4.0 * std::sin(a) - kCampWidth / 2.0;
                    const double dx = tx - obj.x;
                    const double dz = tz - obj.z;
                    const double len = std::sqrt(dx * dx + dz * dz);
                    if (len > 0.001) {
                        obj.dx = speedMax * dx / len;
                        obj.dz = speedMax * dz / len;
                    }
                } else {
                    obj.dx += range(-0.5, 0.5);
                    obj.dz += range(-0.5, 0.5);
                }
                obj.dx = clamp(obj.dx, -speedMax, speedMax);
                obj.dz = clamp(obj.dz, -speedMax, speedMax);
                obj.x += obj.dx;
                obj.z += obj.dz;
                if (obj.x < -kCampWidth / 2.0) {
                    obj.x = -kCampWidth / 2.0;
                    obj.dx = -obj.dx;
                }
                if (obj.x > kCampWidth / 2.0) {
                    obj.x = kCampWidth / 2.0;
                    obj.dx = -obj.dx;
                }
                if (obj.z < -kCampWidth) {
                    obj.z = -kCampWidth;
                    obj.dz = -obj.dz;
                }
                if (obj.z > 0.0) {
                    obj.z = 0.0;
                    obj.dz = -obj.dz;
                }
            }
            campStepAccum_ -= step;
        }
    }

    void updateClouds(double dt)
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        const double skyH = cloudSkyHeight(h);
        constexpr double step = 0.020;
        cloudStepAccum_ = std::min(cloudStepAccum_ + dt, step * 8.0);
        auto sign = [](int v) { return (v > 0) - (v < 0); };
        while (cloudStepAccum_ >= step) {
            for (CloudPen &pen : cloudPens_) {
                for (SDL_Point &point : pen.points) {
                    point.x = static_cast<int>(clamp(point.x + irange(-1, 1), 0, kCloudPenSize - 1));
                    point.y = static_cast<int>(clamp(point.y + irange(-1, 1), 0, kCloudPenSize - 1));
                }
            }
            for (Cloud &cloud : clouds_) {
                cloud.x += cloud.dx * step;
                cloud.y = clamp(cloud.y + cloud.dy * step, 0.0, 0.7 * skyH);
                cloud.color = static_cast<uint16_t>(clamp(65535.0 * cloud.y / (0.8 * skyH), 0.0, 65535.0));
                for (CloudPoint &point : cloud.points) {
                    int k = irange(-16, 15);
                    if (k == -16) k = -point.x;
                    point.x += sign(k);
                    k = irange(-16, 15);
                    if (k == -16) k = -point.y;
                    point.y += sign(k);
                }
            }
            cloudStepAccum_ -= step;
        }
        (void)w;
    }

    void updateMap(double dt, double t)
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        if (t < mapStepDue_) return;
        mapStepDue_ = t + 0.015;

        constexpr double ae1Min = 0.02;
        constexpr double ae1Max = 0.05;
        constexpr double ae2Min = 0.15;
        constexpr double ae2Max = 0.30;
        auto boundedAngle = [&](double current, double min, double max) {
            double a = wrap(current + (max + min) / 5.0 * (random() - 0.5), -max, max);
            a = clamp(a, -max, max);
            if (a >= 0.0 && a <= min) return min;
            if (a <= 0.0 && a >= -min) return -min;
            return a;
        };

        mapA1_ = boundedAngle(mapA1_, ae1Min, ae1Max);
        const double rx = mapWorldX_ * std::cos(mapA1_) - mapWorldY_ * std::sin(mapA1_);
        const double ry = mapWorldX_ * std::sin(mapA1_) + mapWorldY_ * std::cos(mapA1_);
        mapWorldX_ = rx;
        mapWorldY_ = ry;

        mapA2_ = boundedAngle(mapA2_, ae2Min, ae2Max);
        mapA2Total_ += mapA2_;

        const double cx = w * 0.5;
        const double cy = h * 0.5;
        const double cMin = std::max(1.0, std::min(cx, cy));
        mapWorldX_ = clamp(mapWorldX_ + 6.0 * std::cos(mapA2Total_), -cMin + 10.0, cMin - 10.0);
        mapWorldY_ = clamp(mapWorldY_ + 6.0 * std::sin(mapA2Total_), -cMin + 10.0, cMin - 10.0);

        SDL_FPoint last = mapPath_.empty() ? SDL_FPoint{0.0f, 0.0f} : mapPath_.back();
        SDL_FPoint p{
            static_cast<float>((cx / cMin) * mapWorldX_),
            static_cast<float>((cy / cMin) * mapWorldY_),
        };
        if (p.x < last.x) mapLastLeft_ = true;
        else if (p.x > last.x) mapLastLeft_ = false;
        mapPath_.push_back(p);
        if (mapPath_.size() > 6000) {
            mapPath_.erase(mapPath_.begin(), mapPath_.begin() + 300);
        }
        (void)dt;
    }

    void updateBattle(double dt, double t)
    {
        const bool spaceDown = spaceIsDown();
        if (spaceDown && !battleHeld_) {
            holdBattle();
        } else if (!spaceDown && !battleMouseHeld_ && battleHeld_) {
            releaseBattle(t);
        }

        if (battleHeld_) {
            battleTT_ = 0.0;
            battleT0_ = t;
        } else {
            battleTT_ = clamp(std::pow((t - battleT0_) / 2.0, 4.0), 0.0, 1.0);
        }
        if (battleLast_ > 0.0) {
            const double elapsed = t - battleLast_;
            const double direction = battleTT_ < 0.5 ? -1.0 : 1.0;
            battleShift_ += direction * 50.0 * elapsed;
        }
        (void)dt;
        battleLast_ = t;
    }

    double battleHack(double t, double phase) const
    {
        constexpr double hackPeriod = 0.25;
        double tt = std::fmod(t + phase * hackPeriod, hackPeriod) / hackPeriod;
        tt *= 2.0;
        if (tt > 1.0) tt = 2.0 - tt;
        return tt;
    }

    void updateQuail(double dt, double t)
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        const double skyH = h * 0.60;
        for (auto &q : quail_) {
            if (q.dead) {
                q.x += q.dx * dt;
                q.y += 50.0 * dt;
                if (q.y > skyH) {
                    q.y = skyH;
                    q.dx = 0.0;
                }
            } else {
                q.x += q.dx * dt;
                q.y += q.dy * dt;
                if (!(0.0 < q.y && q.y < skyH - 20.0)) {
                    q.dy = -q.dy;
                    q.y += q.dy * dt;
                }
                if (q.x > 0.0 && random() < dt / 10.0) q.dead = true;
            }
        }
        (void)t;
        (void)w;
    }

    void openMenu()
    {
        flow_ = Flow::Menu;
    }

    int menuShortcutIndex(SDL_Keycode key) const
    {
        if (key >= SDLK_1 && key <= SDLK_9) return static_cast<int>(key - SDLK_1);
        if (key == SDLK_0) return 9;
        return -1;
    }

    bool isModifierKey(SDL_Scancode scancode) const
    {
        switch (scancode) {
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:
        case SDL_SCANCODE_LGUI:
        case SDL_SCANCODE_RGUI:
            return true;
        default:
            return false;
        }
    }

    bool hasPrimaryModifier(SDL_Keymod mod) const
    {
        constexpr SDL_Keymod primaryMods =
            SDL_KMOD_SHIFT | SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI;
        return (mod & primaryMods) != 0;
    }

    bool ignoresAnyKey(SDL_Scancode scancode, SDL_Keymod mod) const
    {
        return isModifierKey(scancode) || hasPrimaryModifier(mod);
    }

    bool handleDocKey(SDL_Scancode scancode)
    {
        constexpr int line = 28;
        if (scancode == SDL_SCANCODE_DOWN) {
            docScroll_ += line;
            return true;
        }
        if (scancode == SDL_SCANCODE_UP) {
            docScroll_ = std::max(0, docScroll_ - line);
            return true;
        }
        if (scancode == SDL_SCANCODE_PAGEDOWN) {
            docScroll_ += line * 12;
            return true;
        }
        if (scancode == SDL_SCANCODE_PAGEUP) {
            docScroll_ = std::max(0, docScroll_ - line * 12);
            return true;
        }
        return false;
    }

    void beginWaterStrike()
    {
        waterDownTime_ = nowSeconds();
        waterDownStroke_ = true;
        waterAutoReleaseAt_ = -1.0;
    }

    void releaseWaterStrike()
    {
        if (!waterDownStroke_) return;
        waterDownStroke_ = false;
        waterUpTime_ = nowSeconds();
    }

    void quickWaterStrike()
    {
        beginWaterStrike();
        waterAutoReleaseAt_ = nowSeconds() + 0.13;
    }

    void holdBattle()
    {
        battleHeld_ = true;
    }

    void releaseBattle(double t = -1.0)
    {
        if (battleHeld_) {
            battleHeld_ = false;
            battleT0_ = t >= 0.0 ? t : nowSeconds();
        }
    }

    bool spaceIsDown() const
    {
        int count = 0;
        const bool *keys = SDL_GetKeyboardState(&count);
        return keys && SDL_SCANCODE_SPACE < count && keys[SDL_SCANCODE_SPACE];
    }

    void beginQuailAnimation()
    {
        quailReading_ = false;
        sceneStart_ = nowSeconds();
        docScroll_ = 0;
        initQuail();
    }

    void moveHoreb(SDL_Scancode scancode)
    {
        constexpr double turn = kPi / 100.0;
        constexpr double stride = 40.0;
        if (scancode == SDL_SCANCODE_LEFT) horebAngle_ -= turn;
        if (scancode == SDL_SCANCODE_RIGHT) horebAngle_ += turn;
        if (scancode == SDL_SCANCODE_UP) {
            horebX_ -= std::sin(horebAngle_) * stride;
            horebZ_ -= std::cos(horebAngle_) * stride;
        }
        if (scancode == SDL_SCANCODE_DOWN) {
            horebX_ += std::sin(horebAngle_) * stride;
            horebZ_ += std::cos(horebAngle_) * stride;
        }
        horebAngle_ = wrap(horebAngle_, -kPi, kPi);
    }

    void advanceGodStage()
    {
        if (mode_ != Mode::God) return;
        if (godStage_ == GodStage::Climb) {
            godStage_ = GodStage::Horeb;
        } else if (godStage_ == GodStage::Horeb) {
            godStage_ = GodStage::Talking;
            godTextLines_ = textAssets_.randomGodText(rng_);
        }
        godStageStart_ = nowSeconds();
    }

    void nextComic()
    {
        comicIndex_ = (comicIndex_ + 1) % static_cast<int>(comicFiles_.size());
    }

    void previousComic()
    {
        comicIndex_ = (comicIndex_ + static_cast<int>(comicFiles_.size()) - 1) %
                      static_cast<int>(comicFiles_.size());
    }

    void generateCourt()
    {
        const std::array<std::string, 3> accused = {"A man", "A woman", "A child"};
        const std::array<std::string, 4> crimes = {
            "commits murder", "commits adultery", "commits blasphemy", "commits idolatry"};
        const std::array<std::string, 4> victims = {"to a man", "to a woman", "to a child", "to an animal"};

        const int a = irange(0, 2);
        const int c = irange(0, 3);
        courtPrompt_ = accused[a] + " " + crimes[c];
        if (c <= 1) courtPrompt_ += " " + victims[irange(0, 3)];
        courtPrompt_ += (irange(0, 4) == 0) ? ", again!" : ".";
    }

    void judge(int choice)
    {
        (void)choice;
        finishScene();
    }

    void render(double t)
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(window_, &w, &h);

        if (showIntro_) {
            drawIntro(w, h, t);
            SDL_RenderPresent(renderer_);
            return;
        }

        if (flow_ == Flow::CampWatch) {
            drawCamp(w, h, t);
            SDL_RenderPresent(renderer_);
            return;
        }

        if (flow_ == Flow::Menu) {
            drawMenu(w, h);
            SDL_RenderPresent(renderer_);
            return;
        }

        switch (mode_) {
        case Mode::Camp: drawCamp(w, h, t); break;
        case Mode::God: drawGod(w, h, t); break;
        case Mode::Clouds: drawCloudScene(w, h, t); break;
        case Mode::Court: drawCourt(w, h); break;
        case Mode::Map: drawMap(w, h); break;
        case Mode::WaterRock: drawWaterRock(w, h, t); break;
        case Mode::Battle: drawBattle(w, h, t); break;
        case Mode::Quail: drawQuail(w, h, t); break;
        case Mode::Comics: drawComics(w, h); break;
        case Mode::Help: drawHelp(w, h); break;
        }

        SDL_RenderPresent(renderer_);
    }

    void drawIntro(int w, int h, double t)
    {
        fill({0, 0, static_cast<float>(w), static_cast<float>(h)}, kBlack);
        if (!sprites_.drawCentered("AESplash.DD", 1, w * 0.5f, h * 0.47f,
                                   w * 0.96f, h * 0.86f) &&
            !sprites_.drawCentered("AfterEgypt.HC", 1, w * 0.5f, h * 0.47f,
                                   w * 0.88f, h * 0.76f)) {
            text(titleFont_, "AfterEgypt", w * 0.5f - 96.0f, h * 0.40f, kYellow);
            drawPerson(w * 0.45, h * 0.58, 2.7, false, {76, 42, 148});
            drawPerson(w * 0.53, h * 0.59, 2.2, true, {58, 106, 174});
        }

        const double age = t - introStart_;
        const std::array<std::string, 4> lines = {
            "Leaving all behind, they fled.",
            "Found themselves in a desert.",
            "God!  We're gonna die!",
            "\"Trust Me!\"",
        };
        if (age >= 0.5) {
            const int index = std::min(3, static_cast<int>((age - 0.5) / 1.5));
            const float stripH = 54.0f;
            const float stripY = static_cast<float>(h) - stripH - 18.0f;
            const bool blinkRed = std::fmod(age * 5.0, 1.0) >= 0.5;
            fill({0.0f, stripY, static_cast<float>(w), stripH}, blinkRed ? kRed : kBlack);
            fill({2.0f, stripY + 2.0f, static_cast<float>(w) - 4.0f, stripH - 4.0f}, kBlack);
            const std::string &msg = lines[static_cast<size_t>(index)];
            const float approxW = static_cast<float>(msg.size()) * 9.0f;
            text(font_, msg, w * 0.5f - approxW * 0.5f, stripY + 17.0f, kYellow);
        }
    }

    void sceneBackdrop(int contentW, int h, bool river = false)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, kSky);
        for (int y = 0; y < h; y += 4) {
            const float mix = static_cast<float>(y) / static_cast<float>(std::max(1, h));
            setDraw(blend(kDeepSky, kSky, mix));
            SDL_RenderLine(renderer_, 0.0f, static_cast<float>(y),
                           static_cast<float>(contentW), static_cast<float>(y));
        }
        const bool drewMountain = sprites_.draw("Mountain.HC", 1, 0.0f, h * 0.56f,
                                                static_cast<float>(contentW) / 640.0f);
        if (!drewMountain) {
            triangle({{0.0f, h * 0.58f}, {contentW * 0.32f, h * 0.25f}, {contentW * 0.62f, h * 0.58f}},
                     {118, 97, 87});
            triangle({{contentW * 0.24f, h * 0.58f}, {contentW * 0.56f, h * 0.2f},
                      {static_cast<float>(contentW), h * 0.58f}},
                     {137, 112, 91});
        }
        fill({0, h * 0.56f, static_cast<float>(contentW), h * 0.44f}, kDesert);
        if (river) {
            triangle({{contentW * 0.42f, h * 0.56f}, {contentW * 0.55f, static_cast<float>(h)},
                      {contentW * 0.73f, static_cast<float>(h)}},
                     {31, 128, 215});
        }
        filledCircle(62.0f, 54.0f, 24.0f, kYellow);
    }

    void drawVersePanel(const std::string &book, const std::string &marker, int lineCount,
                        int w, int maxH = 140)
    {
        const float panelH = static_cast<float>(std::min(maxH, std::max(92, w / 9)));
        fill({0, 0, static_cast<float>(w), panelH}, {104, 225, 235, 238});
        fill({0, 0, static_cast<float>(w), 24.0f}, kBlue);
        fill({0, panelH - 22.0f, static_cast<float>(w), 22.0f}, kYellow);
        const std::vector<std::string> lines = textAssets_.bibleVerse(book, marker, lineCount);
        text(smallFont_, joinLines(lines), 18.0f, 31.0f, kInk, std::max(260, w - 36));
    }

    void drawScrollableDoc(const std::string &title, const std::vector<std::string> &lines,
                           int w, int h, const std::string &footer)
    {
        fill({0, 0, static_cast<float>(w), static_cast<float>(h)}, kYellow);
        fill({0, 0, static_cast<float>(w), 58.0f}, kBlue);
        fill({0, 58.0f, static_cast<float>(w), 14.0f}, {104, 225, 235});
        text(titleFont_, title, 28.0f, 14.0f, kYellow);

        SDL_Rect clip{24, 88, std::max(1, w - 48), std::max(1, h - 146)};
        SDL_SetRenderClipRect(renderer_, &clip);
        text(font_, joinLines(lines), 34.0f, 92.0f - static_cast<float>(docScroll_),
             kInk, std::max(260, w - 68));
        SDL_SetRenderClipRect(renderer_, nullptr);

        fill({0, static_cast<float>(h - 42), static_cast<float>(w), 42.0f}, {104, 225, 235});
        text(smallFont_, footer, 28.0f, static_cast<float>(h - 29), kBlue);
    }

    CampProjection projectCamp(double x, double z, int contentW, int h, double localT) const
    {
        constexpr double pitch = 23.0 * kPi / 180.0;
        const double screenScale = clamp(std::min(contentW / 640.0, h / 480.0), 0.92, 2.2);
        const double campScale = kCampScale * screenScale;
        const double viewY = 200.0 - 100.0 * std::sin(localT);
        const double viewZ = 225.0 + 100.0 * std::cos(localT);

        double tx = x;
        const double ty = -viewY;
        const double tz = z - viewZ;
        const double ry = ty * std::cos(pitch) - tz * std::sin(pitch);
        const double rz = ty * std::sin(pitch) + tz * std::cos(pitch);
        if (rz >= -kCampZClip) return {};

        const double perspective = -campScale / rz;
        CampProjection p;
        p.visible = true;
        p.x = static_cast<float>(contentW * 0.5 + tx * perspective);
        p.y = static_cast<float>(h * 0.5 - ry * perspective);
        p.scale = static_cast<float>(0.5 * campScale / -rz);
        p.depth = -rz;
        return p;
    }

    void campLine(double x1, double z1, double x2, double z2, int contentW, int h, double localT,
                  Color color)
    {
        const CampProjection a = projectCamp(x1, z1, contentW, h, localT);
        const CampProjection b = projectCamp(x2, z2, contentW, h, localT);
        if (!a.visible || !b.visible) return;
        thickLine(a.x, a.y, b.x, b.y, 2.0f, color);
    }

    void drawCamp(int contentW, int h, double t)
    {
        const double localT = t - campStart_;
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, kYellow);

        const CampProjection mountain = projectCamp(0.0, -16.0 * kCampWidth, contentW, h, localT);
        if (mountain.visible) {
            const float mountainScale = static_cast<float>(std::max(0.8, std::min(contentW / 640.0, 2.6)));
            if (!sprites_.draw("Camp.HC", 9, 0.0f, mountain.y, mountainScale) &&
                !sprites_.draw("Mountain.HC", 1, 0.0f, mountain.y, mountainScale)) {
                triangle({{0.0f, mountain.y + 58.0f * mountainScale},
                          {contentW * 0.32f, mountain.y - 128.0f * mountainScale},
                          {contentW * 0.62f, mountain.y + 58.0f * mountainScale}},
                         {118, 97, 87});
                triangle({{contentW * 0.24f, mountain.y + 58.0f * mountainScale},
                          {contentW * 0.56f, mountain.y - 144.0f * mountainScale},
                          {static_cast<float>(contentW), mountain.y + 58.0f * mountainScale}},
                         {137, 112, 91});
            }
        }

        campLine(-kCampWidth / 2.0, 0.0, kCampWidth / 2.0, 0.0, contentW, h, localT, {205, 205, 190});
        campLine(-kCampWidth / 2.0, -kCampWidth, kCampWidth / 2.0, -kCampWidth, contentW, h, localT,
                 {205, 205, 190});
        campLine(-kCampWidth / 2.0, 0.0, -kCampWidth / 2.0, -kCampWidth, contentW, h, localT,
                 {205, 205, 190});
        campLine(kCampWidth / 2.0, 0.0, kCampWidth / 2.0, -kCampWidth, contentW, h, localT,
                 {205, 205, 190});

        struct CampDrawItem {
            const CampObj *obj = nullptr;
            CampProjection p;
            size_t index = 0;
        };
        std::vector<CampDrawItem> drawList;
        drawList.reserve(camp_.size());
        for (size_t i = 0; i < camp_.size(); ++i) {
            const CampObj &obj = camp_[i];
            CampProjection p = projectCamp(obj.x, obj.z, contentW, h, localT);
            if (p.visible) drawList.push_back({&obj, p, i});
        }
        std::sort(drawList.begin(), drawList.end(), [](const CampDrawItem &a, const CampDrawItem &b) {
            return a.p.depth > b.p.depth;
        });

        if (campShowCalf_ && std::fmod(localT, 10.0) >= 5.0) {
            const CampProjection calf = projectCamp(0.0, -kCampWidth / 2.0, contentW, h, localT);
            if (calf.visible) {
                const float calfScale = std::max(0.22f, calf.scale);
                if (!sprites_.draw("Camp.HC", 8, calf.x, calf.y, calfScale)) {
                    drawCalf(calf.x, calf.y);
                }
            }
        }

        const double frame = 4.0 * std::fmod(t, 0.5) / 0.5;
        const int frameBase = static_cast<int>(std::floor(frame));
        const double frameFrac = frame - frameBase;
        for (const CampDrawItem &item : drawList) {
            const CampObj &obj = *item.obj;
            const float scale = std::max(0.18f, item.p.scale);
            if (obj.tent) {
                if (!sprites_.draw("Camp.HC", 7, item.p.x, item.p.y, scale)) {
                    drawTent(item.p.x, item.p.y, scale * 1.8f);
                }
            } else {
                const std::array<int, 4> right = {2, 1, 3, 1};
                const std::array<int, 4> left = {5, 4, 6, 4};
                const auto &frames = obj.dx < 0.0 ? left : right;
                const int idx0 = (static_cast<int>(item.index) + frameBase) & 3;
                const int idx1 = (static_cast<int>(item.index) + frameBase + 1) & 3;
                bool drew = false;
                if (frameFrac > 0.02) {
                    const uint8_t alpha1 = static_cast<uint8_t>(clamp((1.0 - frameFrac) * 255.0, 0.0, 255.0));
                    const uint8_t alpha2 = static_cast<uint8_t>(clamp(frameFrac * 255.0, 0.0, 255.0));
                    drew = sprites_.draw("Camp.HC", frames[idx0], item.p.x, item.p.y, scale, alpha1);
                    drew = sprites_.draw("Camp.HC", frames[idx1], item.p.x, item.p.y, scale, alpha2) || drew;
                } else {
                    drew = sprites_.draw("Camp.HC", frames[idx0], item.p.x, item.p.y, scale);
                }
                if (!drew) {
                    drawPerson(item.p.x, item.p.y, scale * 1.8f, obj.dx < 0.0, {82, 48, 164});
                }
            }
        }

        if (campShowCalf_ && std::fmod(localT, 10.0) >= 5.0 && std::fmod(t, 0.7) < 0.46) {
            text(font_, "!!Golden Calf!!", contentW * 0.5f - 76.0f, h * 0.5f - 48.0f, kRed);
        }
    }

    void drawGod(int contentW, int h, double t)
    {
        if (godStage_ == GodStage::Horeb) {
            drawHoreb(contentW, h, t);
            return;
        }
        if (godStage_ == GodStage::Talking) {
            drawGodTalking(contentW, h, t);
            return;
        }

        sceneBackdrop(contentW, h);
        const std::vector<SDL_FPoint> path = mountainPath(contentW, h);
        setDraw(kBrown);
        for (size_t i = 1; i < path.size(); ++i) {
            thickLine(path[i - 1].x, path[i - 1].y, path[i].x, path[i].y, 3.0f, kBrown);
        }

        const double progress = clamp((t - mountainStart_) / 7.5, 0.0, 1.0);
        const SDL_FPoint moses = pathPoint(path, progress);
        const std::array<int, 4> climb = {2, 3, 4, 3};
        const int climbFrame = climb[static_cast<size_t>(std::floor(t * 6.0)) & 3];
        if (!sprites_.draw("Mountain.HC", climbFrame, moses.x, moses.y, 1.65f)) {
            drawPerson(moses.x, moses.y, 1.25, false, {76, 42, 148});
        }
        text(font_, "Mt. Horeb", contentW * 0.5f - 44.0f, h * 0.16f, kInk);

        text(font_, "Moses walks the mountain path.", 42.0f, 44.0f, kInk);
    }

    void drawGodTalking(int contentW, int h, double t)
    {
        const float scale = static_cast<float>(clamp(std::min(contentW / 640.0, h / 480.0), 0.85, 1.25));
        const float rowH = 16.0f * scale;
        const float cyanH = 10.0f * rowH;
        const float mountainY = 20.0f * rowH;
        const float textY = 22.0f * rowH;

        fill({0, 0, static_cast<float>(contentW), cyanH}, {118, 218, 230});
        fill({0, cyanH, static_cast<float>(contentW), static_cast<float>(h) - cyanH}, kYellow);

        const float backdropScale = std::max(scale, static_cast<float>(contentW) / 640.0f);
        if (!sprites_.draw("GodTalking.HC", 4, 0.0f, mountainY, backdropScale) &&
            !sprites_.draw("Mountain.HC", 1, 0.0f, mountainY, backdropScale)) {
            triangle({{0.0f, mountainY + 48.0f * scale},
                      {contentW * 0.28f, mountainY - 116.0f * scale},
                      {contentW * 0.58f, mountainY + 48.0f * scale}},
                     {112, 92, 82});
            triangle({{contentW * 0.24f, mountainY + 48.0f * scale},
                      {contentW * 0.57f, mountainY - 136.0f * scale},
                      {static_cast<float>(contentW), mountainY + 48.0f * scale}},
                     {134, 106, 82});
        }

        const float ox = 0.0f;
        const float oy = 0.0f;
        const int talkFrame = (std::fmod(t, 0.8) < 0.4) ? 1 : 2;
        if (!sprites_.draw("GodTalking.HC", talkFrame, ox + 44.0f * scale, oy + 99.0f * scale, scale)) {
            drawPerson(ox + 80.0f * scale, oy + 170.0f * scale, 1.8 * scale, false, {76, 42, 148});
        }

        const float bushX = ox + 213.0f * scale;
        const float bushY = oy + 91.0f * scale;
        if (!sprites_.draw("GodTalking.HC", 3, bushX, bushY, scale)) {
            drawBurningBush(bushX, bushY, t);
        }

        for (int i = 0; i < 256; ++i) {
            const double a1 = range(0.0, 2.0 * kPi);
            const double a2 = range(0.0, 2.0 * kPi);
            const double r1 = random() * random();
            const double r2 = random() * random();
            line((235.0f + 30.0f * r1 * std::cos(a1)) * scale,
                 (56.0f + 30.0f * r1 * std::sin(a1)) * scale,
                 (235.0f + 30.0f * r2 * std::cos(a2)) * scale,
                 (40.0f + 30.0f * r2 * std::sin(a2)) * scale,
                 paletteCycle(t + i));
        }

        const double wave = std::fmod(t, 4.0) < 2.0 ? std::sin(t * kPi) : 0.0;
        for (int i = 0; i < 120; ++i) {
            line((i + 10) * scale, (110.0f + 4.0f * wave * std::sin(i / 6.0)) * scale,
                 (i + 11) * scale, (110.0f + 4.0f * wave * std::sin((i + 1) / 6.0)) * scale,
                 kBrown);
        }

        if (godTextLines_.empty()) godTextLines_ = textAssets_.randomGodText(rng_);
        std::vector<std::string> body = godTextLines_;
        if (!body.empty() && body.front() == "God Says...") body.erase(body.begin());
        fill({0.0f, textY - 8.0f, static_cast<float>(contentW),
              static_cast<float>(h) - textY + 8.0f},
             kYellow);
        text(font_, "God Says...", 18.0f, textY, kBlue);
        text(smallFont_, joinLines(body), 18.0f, textY + 30.0f, {150, 0, 0},
             std::max(260, contentW - 36));
        text(smallFont_, "Press <ESC>.", 18.0f, h - 34.0f, kBlue);
    }

    HorebProjection projectHoreb(const HorebObj &obj, int contentW, int h) const
    {
        const double x0 = obj.x + horebX_;
        const double y0 = obj.y;
        const double z0 = obj.z + horebZ_;
        const double side = x0 * std::cos(horebAngle_) - z0 * std::sin(horebAngle_);
        const double depth = x0 * std::sin(horebAngle_) + z0 * std::cos(horebAngle_);
        constexpr double pitch = 77.0 * kPi / 180.0;
        const double y1 = y0 * std::cos(pitch) - depth * std::sin(pitch);
        const double z1 = y0 * std::sin(pitch) + depth * std::cos(pitch);
        if (z1 <= 0.0) return {};

        const double s = 100.0 / (std::abs(z1) + 50.0);
        const float x = contentW * 0.5f + static_cast<float>(side * s);
        const float y = static_cast<float>(h + y1 * s);
        if (x < -220.0f || x > contentW + 220.0f || y < -160.0f || y > h + 120.0f) return {};

        HorebProjection p;
        p.visible = true;
        p.x = x;
        p.y = y;
        p.scale = static_cast<float>(clamp(s * 2.0, 0.08, 2.15));
        p.depth = z1;
        p.side = side;
        return p;
    }

    void drawHorebObject(const HorebObj &obj, const HorebProjection &p, double t)
    {
        if (obj.bi == 0) {
            static constexpr std::array<Color, 4> pebbleColors = {
                kBlack, Color{74, 74, 74}, Color{74, 74, 74}, Color{170, 170, 170},
            };
            line(p.x, p.y, p.x + 1.0f, p.y, pebbleColors[static_cast<size_t>(obj.seed & 3)]);
            return;
        }

        float scale = p.scale;
        if (obj.seed == 0) scale *= 1.2f;

        const bool mirror = (obj.bi == 6 || obj.bi == 7 || obj.bi == 8) && obj.mirror;
        if (!sprites_.draw("HorebA.HC", obj.bi, p.x, p.y, scale, 255, mirror)) {
            if (obj.bi == 1 || obj.bi == 2) {
                line(p.x, p.y, p.x - 12.0f * scale, p.y + 30.0f * scale, kBrown);
                line(p.x, p.y, p.x + 12.0f * scale, p.y + 30.0f * scale, kBrown);
                filledCircle(p.x - 8.0f * scale, p.y - 10.0f * scale, 9.0f * scale, {45, 154, 65});
                filledCircle(p.x + 8.0f * scale, p.y - 8.0f * scale, 8.0f * scale, {54, 184, 73});
            } else if (obj.bi == 3) {
                thickLine(p.x - 18.0f * scale, p.y + 16.0f * scale,
                          p.x + 20.0f * scale, p.y + 4.0f * scale,
                          std::max(1.0f, 4.0f * scale), kBrown);
            } else if (obj.bi == 4 || obj.bi == 5) {
                line(p.x, p.y, p.x + std::sin(obj.seed) * 18.0f * scale, p.y - 42.0f * scale, kBrown);
                filledCircle(p.x, p.y - 34.0f * scale, 12.0f * scale, {39, 108, 63});
            } else if (obj.bi >= 6) {
                drawPerson(p.x, p.y, scale * 0.9, mirror, {236, 236, 220});
            }
        }

        if (obj.seed == 0) {
            for (int i = 0; i < 45; ++i) {
                const double a1 = t * 4.0 + i * 1.7;
                const double a2 = t * 3.2 + i * 2.1;
                const double r1 = std::sqrt(std::abs(std::sin(i * 12.3))) * 22.0 * scale;
                const double r2 = std::sqrt(std::abs(std::cos(i * 8.7))) * 22.0 * scale;
                line(p.x + std::cos(a1) * r1, p.y - 24.0f * scale + std::sin(a1) * r1,
                     p.x + std::cos(a2) * r2, p.y - 24.0f * scale + std::sin(a2) * r2,
                     paletteCycle(t + i));
            }
        }
    }

    void drawHorebDocument(int contentW, int h)
    {
        const float rowH = static_cast<float>(clamp(h / 72.0, 8.0, 12.0));
        const float cyanH = rowH * 6.0f;
        const float verseY = rowH * 36.0f;
        fill({0, 0, static_cast<float>(contentW), cyanH}, {104, 225, 235});
        fill({0, cyanH, static_cast<float>(contentW), static_cast<float>(h) - cyanH}, kYellow);
        text(smallFont_, joinLines(textAssets_.bibleVerse("Exodus", "3:1", 21)),
             18.0f, verseY, kBlue, std::max(260, contentW - 36));
    }

    void drawHoreb(int contentW, int h, double t)
    {
        drawHorebDocument(contentW, h);

        const float cx = contentW * 0.5f;
        const double sunX0 = horebX_;
        const double sunZ0 = 1000000.0 + horebZ_;
        const double sunSide = sunX0 * std::cos(horebAngle_) - sunZ0 * std::sin(horebAngle_);
        const double sunDepth = sunX0 * std::sin(horebAngle_) + sunZ0 * std::cos(horebAngle_);
        const double sunY = -std::sin(77.0 * kPi / 180.0) * sunDepth;
        const double sunZ = std::cos(77.0 * kPi / 180.0) * sunDepth;
        if (sunY < 0.0 && sunZ > 0.0) {
            const double sunS = 100.0 / (std::abs(sunZ) + 50.0);
            const float sunX = cx + static_cast<float>(sunSide * sunS);
            filledCircle(sunX, 15.0f, 17.0f, kBrown);
            filledCircle(sunX, 15.0f, 15.0f, kYellow);
        }

        struct DrawItem {
            HorebObj obj;
            HorebProjection p;
        };
        std::vector<DrawItem> draws;
        draws.reserve(horebObjs_.size());
        for (const HorebObj &obj : horebObjs_) {
            HorebProjection p = projectHoreb(obj, contentW, h);
            if (p.visible) draws.push_back({obj, p});
        }
        std::sort(draws.begin(), draws.end(), [](const DrawItem &a, const DrawItem &b) {
            return a.p.depth > b.p.depth;
        });
        for (const DrawItem &item : draws) {
            drawHorebObject(item.obj, item.p, t);
        }

        if (!horebObjs_.empty()) {
            const HorebProjection bush = projectHoreb(horebObjs_.front(), contentW, h);
            const double screenDistance = bush.visible
                                              ? std::hypot(bush.x - cx, bush.y - static_cast<float>(h))
                                              : 100000.0;
            if (!horebFound_ && bush.visible && screenDistance < 300.0) {
                horebFound_ = true;
                horebFoundTime_ = nowSeconds();
            }
        }

        if (!horebFound_ && std::fmod(t, 1.0) < 0.58) {
            text(font_, "Find the Burning Bush.", contentW * 0.5f - 104.0f,
                 h * 0.5f - 11.0f, kRed);
        }
    }

    void drawCloudScene(int contentW, int h, double t)
    {
        const float rowH = static_cast<float>(clamp(h / 36.0, 12.0, 16.0));
        const float skyH = static_cast<float>(cloudSkyHeight(h));
        const float yellowStart = skyH;
        const float verseY = yellowStart + rowH * 5.0f;
        fill({0, 0, static_cast<float>(contentW), skyH}, {104, 225, 235});
        fill({0, yellowStart, static_cast<float>(contentW), static_cast<float>(h) - yellowStart}, kYellow);
        if (!sprites_.draw("Clouds.HC", 1, 0.0f, skyH, static_cast<float>(contentW) / 640.0f) &&
            !sprites_.draw("Mountain.HC", 1, 0.0f, skyH, static_cast<float>(contentW) / 640.0f)) {
            triangle({{0.0f, skyH + 58.0f}, {contentW * 0.32f, skyH - 128.0f},
                      {contentW * 0.62f, skyH + 58.0f}},
                     {118, 97, 87});
            triangle({{contentW * 0.24f, skyH + 58.0f}, {contentW * 0.56f, skyH - 144.0f},
                      {static_cast<float>(contentW), skyH + 58.0f}},
                     {137, 112, 91});
        }
        for (const auto &cloud : clouds_) drawCloud(cloud);
        text(smallFont_, joinLines(textAssets_.bibleVerse("Exodus", "14:19", 7)),
             18.0f, verseY, kBlue, std::max(260, contentW - 36));
        (void)t;
    }

    void drawCourt(int contentW, int h)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, {38, 43, 55});
        sceneBackdrop(contentW, h);
        fill({contentW * 0.1f, h * 0.16f, contentW * 0.8f, h * 0.52f}, {236, 219, 172, 235});
        text(titleFont_, "Hold Court", contentW * 0.14f, h * 0.2f, kInk);
        text(font_, courtPrompt_, contentW * 0.14f, h * 0.29f, kInk, static_cast<int>(contentW * 0.66));
        const auto rects = courtRects(contentW, h);
        const std::array<std::string, 3> labels = {"Show Mercy", "Punish", "Really Punish"};
        const std::array<Color, 3> colors = {kGreen, kGold, kRed};
        for (size_t i = 0; i < rects.size(); ++i) {
            fill(rects[i], colors[i]);
            text(font_, labels[i], rects[i].x + 16.0f, rects[i].y + 12.0f, kBlack);
        }
    }

    void drawMap(int contentW, int h)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, kYellow);

        const float cx = contentW * 0.5f;
        const float cy = h * 0.5f;
        setDraw(kInk);
        for (size_t i = 1; i < mapPath_.size(); i += 2) {
            line(cx + mapPath_[i - 1].x, cy + mapPath_[i - 1].y,
                 cx + mapPath_[i].x, cy + mapPath_[i].y, kBlack);
        }
        if (!mapPath_.empty()) {
            const SDL_FPoint p = mapPath_.back();
            const std::array<int, 4> right = {2, 3, 4, 3};
            const std::array<int, 4> leftFrames = {5, 6, 7, 6};
            const int frame = (static_cast<int>(std::floor(nowSeconds() * 6.0)) +
                               static_cast<int>(mapPath_.size())) & 3;
            const int bi = mapLastLeft_ ? leftFrames[frame] : right[frame];
            if (!sprites_.draw("Mountain.HC", bi, cx + p.x, cy + p.y, 1.8f)) {
                drawPerson(cx + p.x, cy + p.y, 1.15, mapLastLeft_, {76, 42, 148});
            }
        }
        text(smallFont_, joinLines(textAssets_.bibleVerse("Exodus", "16:35", 3)),
             18.0f, 24.0f, kBlue, std::max(260, contentW - 36));
    }

    void drawWaterRock(int contentW, int h, double t)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, kYellow);

        const float sceneScale = static_cast<float>(clamp(std::min(contentW / 640.0, h / 480.0), 0.85, 1.8));
        const float cx = contentW * 0.5f;
        const float cy = h * 0.5f;
        const float rockX = cx - 64.0f * sceneScale;
        const float rockY = cy - 4.0f * sceneScale;
        const float waterX = cx - 63.0f * sceneScale;
        const float waterY = cy - 20.0f * sceneScale;

        constexpr double downDelay = 0.075;
        constexpr double upTime = 0.2;
        constexpr double spreadRate = 5.0;
        const bool waterMade = waterDownTime_ >= 0.0 && t - waterDownTime_ >= downDelay;
        const double age = waterMade ? t - waterDownTime_ - downDelay : -1.0;
        if (waterMade) {
            const float r = static_cast<float>(std::min(17.0, spreadRate * age) * sceneScale);
            if (r > 0.0f) {
                ring(waterX, waterY, std::max(1.0f, r), kBlue);
                if (r >= 2.0f * sceneScale) {
                    filledCircle(waterX, waterY, r - 1.0f, kBlue);
                }
            }
        }

        float arm = 0.0f;
        if (waterDownStroke_ && waterDownTime_ >= 0.0) {
            arm = static_cast<float>(clamp((t - waterDownTime_) / downDelay, 0.0, 1.0));
        } else if (waterUpTime_ >= 0.0) {
            arm = static_cast<float>(1.0 - clamp((t - waterUpTime_) / upTime, 0.0, 1.0));
        }
        int mosesBi = 1;
        if (arm > 0.66f) mosesBi = 4;
        else if (arm > 0.20f) mosesBi = 3;
        else if (std::fmod(t, 1.2) >= 0.6) mosesBi = 2;
        if (!sprites_.draw("WaterRock.HC", mosesBi, cx, cy, sceneScale)) {
            drawPerson(cx, cy + 10.0f * sceneScale, 1.35 * sceneScale, true, {76, 42, 148});
            line(cx - 18.0f * sceneScale, cy - 28.0f * sceneScale + arm * 24.0f * sceneScale,
                 cx - 70.0f * sceneScale, cy - 70.0f * sceneScale + arm * 76.0f * sceneScale, kBrown);
        }
        if (!sprites_.draw("WaterRock.HC", 5, rockX, rockY, sceneScale)) {
            fill({rockX - 26.0f * sceneScale, rockY - 10.0f * sceneScale,
                  52.0f * sceneScale, 34.0f * sceneScale},
                 kRock);
            triangle({{rockX - 42.0f * sceneScale, rockY + 28.0f * sceneScale},
                      {rockX - 14.0f * sceneScale, rockY - 30.0f * sceneScale},
                      {rockX + 30.0f * sceneScale, rockY + 28.0f * sceneScale}},
                     kRock);
        }
        drawVersePanel("Exodus", "17:6", 4, contentW, 118);
        text(font_, "<SPACE>", 34.0f, 126.0f, kInk);
    }

    void drawBattle(int contentW, int h, double t)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, kYellow);
        const float sceneScale = static_cast<float>(clamp(std::min(contentW / 640.0, h / 480.0), 0.85, 1.8));
        const float cx = contentW * 0.5f;
        const float cy = h * 0.5f;
        const float spacing = 45.0f * sceneScale;
        const bool handsUp = battleTT_ < 0.5;
        const int mosesBi = handsUp ? 3 : 4;
        if (!sprites_.draw("Battle.HC", mosesBi, cx, cy + spacing, sceneScale)) {
            drawMosesArms(cx, cy + spacing - 54.0f * sceneScale, handsUp);
        }

        const float offset = static_cast<float>(battleShift_);
        const std::array<SDL_FPoint, 3> fighters = {{
            {cx + offset + spacing, cy - spacing},
            {cx + offset + 2.0f * spacing, cy - spacing},
            {cx + offset + spacing, cy - 2.0f * spacing},
        }};
        const std::array<double, 3> phase = {0.0, 0.333, 0.666};
        for (size_t i = 0; i < fighters.size(); ++i) {
            const double saw = battleHack(t, phase[i]);
            const int bi = saw > 0.5 ? 2 : 1;
            if (!sprites_.draw("Battle.HC", bi, fighters[i].x, fighters[i].y, sceneScale)) {
                drawFighter(fighters[i].x, fighters[i].y, true, t + i);
            }
        }

        drawVersePanel("Exodus", "17:11", 8, contentW, 132);
        text(font_, "Hold <SPACE>", 34.0f, 140.0f, kInk);
    }

    void drawQuail(int contentW, int h, double t)
    {
        if (quailReading_) {
            drawScrollableDoc("Numbers 11:11",
                              textAssets_.bibleVerse("Numbers", "11:11", 88),
                              contentW, h,
                              "Scroll down to finish reading. Enter/Space begins. Esc returns.");
            return;
        }

        const float skyH = h * 0.60f;
        const float sceneScale = static_cast<float>(clamp(std::min(contentW / 640.0, h / 480.0), 0.85, 1.8));
        fill({0, 0, static_cast<float>(contentW), skyH}, {104, 225, 235});
        fill({0, skyH, static_cast<float>(contentW), h - skyH}, kYellow);
        if (!sprites_.draw("Quail.HC", 4, 0.0f, skyH, static_cast<float>(contentW) / 640.0f) &&
            !sprites_.draw("Mountain.HC", 1, 0.0f, skyH, static_cast<float>(contentW) / 640.0f)) {
            triangle({{0.0f, skyH + 58.0f}, {contentW * 0.32f, skyH - 128.0f},
                      {contentW * 0.62f, skyH + 58.0f}},
                     {118, 97, 87});
            triangle({{contentW * 0.24f, skyH + 58.0f}, {contentW * 0.56f, skyH - 144.0f},
                      {static_cast<float>(contentW), skyH + 58.0f}},
                     {137, 112, 91});
        }
        const double t1 = t - sceneStart_;
        for (const auto &q : quail_) {
            if (q.dead) {
                if (!sprites_.draw("Quail.HC", 3, static_cast<float>(q.x), static_cast<float>(q.y), sceneScale)) {
                    drawDeadBird(q.x, q.y);
                }
            } else {
                double flap = std::fmod(t1 + q.phase, 1.0);
                if (flap < 0.0) flap += 1.0;
                flap *= 2.0;
                if (flap > 1.0) flap = 2.0 - flap;
                const int bi = flap > 0.5 ? 2 : 1;
                if (!sprites_.draw("Quail.HC", bi, static_cast<float>(q.x), static_cast<float>(q.y), sceneScale)) {
                    drawBird(q.x, q.y, 8.0 + 5.0 * flap);
                }
            }
        }
        text(font_, "Press any key.", 28.0f, h - 42.0f, kInk);
    }

    void drawComics(int contentW, int h)
    {
        fill({0, 0, static_cast<float>(contentW), static_cast<float>(h)}, {46, 44, 51});
        if (comicPicker_) {
            fill({0, 0, static_cast<float>(contentW), 58.0f}, kBlue);
            fill({0, 58.0f, static_cast<float>(contentW), h - 58.0f}, kYellow);
            text(titleFont_, "Moses Comics", 28.0f, 14.0f, kYellow);
            const float bw = std::min(230.0f, contentW * 0.26f);
            const float bh = 44.0f;
            const float gap = 16.0f;
            const float gridW = bw * 3.0f + gap * 2.0f;
            const float x0 = (contentW - gridW) * 0.5f;
            const float y0 = h * 0.22f;
            for (size_t i = 0; i < comicFiles_.size(); ++i) {
                const int col = static_cast<int>(i % 3);
                const int row = static_cast<int>(i / 3);
                SDL_FRect rect{x0 + col * (bw + gap), y0 + row * (bh + gap), bw, bh};
                fill(rect, i == static_cast<size_t>(comicIndex_) ? kGold : kPanelLite);
                text(font_, comicName(static_cast<int>(i)), rect.x + 14.0f, rect.y + 12.0f,
                     i == static_cast<size_t>(comicIndex_) ? kBlack : kYellow);
            }
            text(smallFont_, "Select a file. Esc returns.", 28.0f, h - 34.0f, kBlue);
            return;
        }

        const SDL_FRect panel{contentW * 0.08f, h * 0.10f, contentW * 0.84f, h * 0.72f};
        fill(panel, {244, 228, 185});
        fill({panel.x + 10, panel.y + 10, panel.w - 20, panel.h - 20}, {236, 201, 119});
        drawComicArt(panel, comicIndex_);
        text(titleFont_, comicName(comicIndex_), panel.x + 26.0f, panel.y + panel.h + 18.0f, kWhite);
        text(font_, std::to_string(comicIndex_ + 1) + " / " + std::to_string(comicFiles_.size()),
             contentW * 0.5f - 20.0f, h - 42.0f, kWhite);
        text(smallFont_, "Left/right changes file. Esc returns to picker.", 28.0f, h - 34.0f, kWhite);
    }

    void drawMenu(int w, int h)
    {
        fill({0, 0, static_cast<float>(w), static_cast<float>(h)}, kBlue);
        fill({0, 0, static_cast<float>(w), h * 0.18f}, {90, 215, 232});
        fill({0, h * 0.18f, static_cast<float>(w), h * 0.82f}, kYellow);
        if (sprites_.drawCentered("AESplash.DD", 1, w * 0.5f, h * 0.17f,
                                  w * 0.62f, h * 0.26f)) {
            fill({0, h * 0.30f, static_cast<float>(w), 2.0f}, kBlue);
        } else {
            text(titleFont_, "AfterEgypt", w * 0.5f - 94.0f, 38.0f, kBlue);
        }

        for (const auto &button : layoutButtons(w, h)) {
            fill(button.rect, button.action == Action::Quit ? kRed : kPanelLite);
            text(font_, button.label, button.rect.x + 16.0f, button.rect.y + 12.0f,
                 button.action == Action::Quit ? kWhite : kYellow);
        }

        text(smallFont_, "Camp " + std::to_string(day_) + "  People " + std::to_string(people_),
             28.0f, h - 38.0f, kBlue);
        text(smallFont_, audio_.muted() ? "M unmutes audio" : "M mutes audio",
             static_cast<float>(w) - 160.0f, h - 38.0f, kBlue);
    }

    std::vector<MenuButton> layoutButtons(int w, int h) const
    {
        const float bw = std::min(310.0f, std::max(220.0f, w * 0.32f));
        constexpr float bh = 46.0f;
        constexpr float gapX = 22.0f;
        constexpr float gapY = 12.0f;
        const float totalW = bw * 2.0f + gapX;
        const float x0 = (static_cast<float>(w) - totalW) * 0.5f;
        const float y0 = std::max(130.0f, h * 0.28f);

        std::vector<MenuButton> out;
        auto add = [&](int index, const std::string &label, Action action) {
            const int col = index % 2;
            const int row = index / 2;
            const float x = x0 + col * (bw + gapX);
            const float y = y0 + row * (bh + gapY);
            out.push_back({{x, y, bw, bh}, label, action});
        };
        add(0, "1  Break Camp", Action::BreakCamp);
        add(1, "2  Talk with God", Action::God);
        add(2, "3  View Clouds", Action::Clouds);
        add(3, "4  Hold Court", Action::Court);
        add(4, "5  View Map", Action::Map);
        add(5, "6  Make Water", Action::WaterRock);
        add(6, "7  Battle", Action::Battle);
        add(7, "8  Beg for Meat", Action::Quail);
        add(8, "9  Moses Comics", Action::Comics);
        add(9, "0  Help", Action::Help);
        out.push_back({{x0 + bw * 0.5f + gapX * 0.5f, y0 + 5.0f * (bh + gapY),
                        bw, bh},
                       "Q  Quit", Action::Quit});
        return out;
    }

    bool actionActive(Action action) const
    {
        switch (action) {
        case Action::BreakCamp: return mode_ == Mode::Camp;
        case Action::God: return mode_ == Mode::God;
        case Action::Clouds: return mode_ == Mode::Clouds;
        case Action::Court: return mode_ == Mode::Court;
        case Action::Map: return mode_ == Mode::Map;
        case Action::WaterRock: return mode_ == Mode::WaterRock;
        case Action::Battle: return mode_ == Mode::Battle;
        case Action::Quail: return mode_ == Mode::Quail;
        case Action::Comics: return mode_ == Mode::Comics;
        case Action::Help: return mode_ == Mode::Help;
        case Action::Quit: return false;
        }
        return false;
    }

    std::string modeName() const
    {
        switch (mode_) {
        case Mode::Camp: return "Camp";
        case Mode::God: return "Talk with God";
        case Mode::Clouds: return "View Clouds";
        case Mode::Court: return "Hold Court";
        case Mode::Map: return "View Map";
        case Mode::WaterRock: return "Make Water";
        case Mode::Battle: return "Battle";
        case Mode::Quail: return "Beg for Meat";
        case Mode::Comics: return "Moses Comics";
        case Mode::Help: return "Help";
        }
        return "";
    }

    void setDraw(Color color)
    {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    }

    void fill(SDL_FRect rect, Color color)
    {
        setDraw(color);
        SDL_RenderFillRect(renderer_, &rect);
    }

    void line(double x1, double y1, double x2, double y2, Color color)
    {
        setDraw(color);
        SDL_RenderLine(renderer_, static_cast<float>(x1), static_cast<float>(y1),
                       static_cast<float>(x2), static_cast<float>(y2));
    }

    void thickLine(float x1, float y1, float x2, float y2, float thickness, Color color)
    {
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
        const float nx = -dy / len * thickness * 0.5f;
        const float ny = dx / len * thickness * 0.5f;
        triangle({{x1 + nx, y1 + ny}, {x2 + nx, y2 + ny}, {x2 - nx, y2 - ny}}, color);
        triangle({{x1 + nx, y1 + ny}, {x2 - nx, y2 - ny}, {x1 - nx, y1 - ny}}, color);
    }

    void triangle(std::initializer_list<SDL_FPoint> pts, Color color)
    {
        if (pts.size() != 3) return;
        const SDL_FColor c = fcolor(color);
        SDL_Vertex verts[3];
        int i = 0;
        for (const SDL_FPoint &p : pts) {
            verts[i++] = {p, c, {0, 0}};
        }
        SDL_RenderGeometry(renderer_, nullptr, verts, 3, nullptr, 0);
    }

    void filledCircle(float cx, float cy, float r, Color color)
    {
        setDraw(color);
        const int radius = static_cast<int>(std::ceil(r));
        for (int dy = -radius; dy <= radius; ++dy) {
            const float span = std::sqrt(std::max(0.0f, r * r - static_cast<float>(dy * dy)));
            SDL_RenderLine(renderer_, cx - span, cy + dy, cx + span, cy + dy);
        }
    }

    void ring(float cx, float cy, float r, Color color)
    {
        setDraw(color);
        constexpr int steps = 72;
        for (int i = 0; i < steps; ++i) {
            const double a1 = 2.0 * kPi * i / steps;
            const double a2 = 2.0 * kPi * (i + 1) / steps;
            line(cx + std::cos(a1) * r, cy + std::sin(a1) * r,
                 cx + std::cos(a2) * r, cy + std::sin(a2) * r, color);
        }
    }

    void text(TTF_Font *font, const std::string &s, float x, float y, Color color, int wrapWidth = 0)
    {
        SDL_Color c{color.r, color.g, color.b, color.a};
        SDL_Surface *surface = wrapWidth > 0
                                   ? TTF_RenderText_Blended_Wrapped(font, s.c_str(), s.size(), c, wrapWidth)
                                   : TTF_RenderText_Blended(font, s.c_str(), s.size(), c);
        if (!surface) return;
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
        if (!texture) {
            SDL_DestroySurface(surface);
            return;
        }
        SDL_FRect dst{x, y, static_cast<float>(surface->w), static_cast<float>(surface->h)};
        SDL_RenderTexture(renderer_, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    Color blend(Color a, Color b, float t) const
    {
        return {
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t),
            static_cast<uint8_t>(a.a + (b.a - a.a) * t),
        };
    }

    static bool pointIn(SDL_FRect rect, float x, float y)
    {
        return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
    }

    void drawTent(double x, double y, double s)
    {
        const float w = static_cast<float>(36.0 * s);
        const float h = static_cast<float>(28.0 * s);
        triangle({{static_cast<float>(x - w), static_cast<float>(y)},
                  {static_cast<float>(x), static_cast<float>(y - h)},
                  {static_cast<float>(x + w), static_cast<float>(y)}},
                 {189, 65, 48});
        triangle({{static_cast<float>(x - w * 0.25), static_cast<float>(y)},
                  {static_cast<float>(x), static_cast<float>(y - h * 0.76)},
                  {static_cast<float>(x + w * 0.18), static_cast<float>(y)}},
                 {77, 38, 44});
    }

    void drawPerson(double x, double y, double s, bool left, Color robe)
    {
        const float head = static_cast<float>(4.2 * s);
        const float body = static_cast<float>(13.0 * s);
        const float sx = static_cast<float>(x);
        const float sy = static_cast<float>(y);
        filledCircle(sx, sy - body - head, head, {102, 61, 34});
        triangle({{sx, sy - body}, {sx - body * 0.48f, sy}, {sx + body * 0.48f, sy}}, robe);
        line(sx - body * 0.22f, sy - body * 0.1f, sx - body * 0.45f, sy + body * 0.38f, kBlack);
        line(sx + body * 0.22f, sy - body * 0.1f, sx + body * 0.45f, sy + body * 0.38f, kBlack);
        line(sx, sy - body * 0.75f, sx + (left ? -1.0f : 1.0f) * body * 0.55f, sy - body * 0.42f, kBrown);
    }

    void drawCalf(double x, double y)
    {
        fill({static_cast<float>(x - 34.0), static_cast<float>(y - 18.0), 58.0f, 28.0f}, kGold);
        filledCircle(static_cast<float>(x + 30.0), static_cast<float>(y - 15.0), 14.0f, kGold);
        line(x + 24.0, y - 30.0, x + 15.0, y - 44.0, kGold);
        line(x + 38.0, y - 30.0, x + 50.0, y - 42.0, kGold);
        for (int i = 0; i < 4; ++i) {
            const double lx = x - 20.0 + i * 14.0;
            line(lx, y + 8.0, lx - 4.0, y + 28.0, kBrown);
        }
    }

    void drawCloud(const Cloud &cloud)
    {
        for (size_t i = 0; i < cloud.points.size(); ++i) {
            const CloudPoint &point = cloud.points[i];
            const CloudPen &pen = cloudPens_[i & (kCloudPens - 1)];
            const Color color = point.shade < cloud.color ? Color{170, 170, 170} : kWhite;
            const float baseX = static_cast<float>(cloud.x + point.x);
            const float baseY = static_cast<float>(cloud.y + point.y);
            for (const SDL_Point &penPoint : pen.points) {
                const float x = baseX + static_cast<float>(penPoint.x);
                const float y = baseY + static_cast<float>(penPoint.y);
                line(x, y, x + 1.0f, y, color);
            }
        }
    }

    std::vector<SDL_FPoint> mountainPath(int contentW, int h) const
    {
        const float cx = contentW * 0.5f;
        const float cy = h * 0.69f;
        const float s = std::min(contentW / 900.0f, h / 690.0f);
        const std::array<SDL_FPoint, 9> src = {
            SDL_FPoint{0, 0}, {100, -20}, {-100, -40}, {80, -60}, {-80, -60},
            {60, -80}, {-60, -100}, {40, -120}, {-37, -140},
        };
        std::vector<SDL_FPoint> out;
        for (const auto &p : src) out.push_back({cx + p.x * s * 2.2f, cy + p.y * s * 2.35f});
        return out;
    }

    SDL_FPoint pathPoint(const std::vector<SDL_FPoint> &path, double progress) const
    {
        if (path.empty()) return {0, 0};
        const double scaled = clamp(progress, 0.0, 1.0) * (path.size() - 1);
        const int i = std::min(static_cast<int>(scaled), static_cast<int>(path.size()) - 2);
        const double f = scaled - i;
        return {
            static_cast<float>(path[i].x + (path[i + 1].x - path[i].x) * f),
            static_cast<float>(path[i].y + (path[i + 1].y - path[i].y) * f),
        };
    }

    void drawBurningBush(double x, double y, double t)
    {
        line(x, y, x - 22.0, y + 38.0, kBrown);
        line(x, y, x + 24.0, y + 38.0, kBrown);
        for (int i = 0; i < 10; ++i) {
            const double a = 2.0 * kPi * i / 10.0 + std::sin(t * 3.0 + i) * 0.15;
            filledCircle(static_cast<float>(x + std::cos(a) * 22.0),
                         static_cast<float>(y + std::sin(a) * 15.0),
                         10.0f, i % 2 ? kRed : kYellow);
        }
    }

    Color paletteCycle(double t) const
    {
        const int i = static_cast<int>(std::floor(t * 7.0)) & 3;
        if (i == 0) return kYellow;
        if (i == 1) return kWater;
        if (i == 2) return kPurple;
        return kRed;
    }

    std::array<SDL_FRect, 3> courtRects(int contentW, int h) const
    {
        const float x = contentW * 0.14f;
        const float y = h * 0.39f;
        const float w = contentW * 0.21f;
        return {{
            {x, y, w, 48.0f},
            {x + w + 18.0f, y, w, 48.0f},
            {x + 2.0f * (w + 18.0f), y, w + 24.0f, 48.0f},
        }};
    }

    int courtChoiceAt(float x, float y, int contentW, int h) const
    {
        const auto rects = courtRects(contentW, h);
        for (size_t i = 0; i < rects.size(); ++i) {
            if (pointIn(rects[i], x, y)) return static_cast<int>(i);
        }
        return -1;
    }

    std::string comicName(int index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= comicFiles_.size()) return "";
        return fs::path(comicFiles_[static_cast<size_t>(index)]).stem().string();
    }

    int comicChoiceAt(float x, float y, int w, int h) const
    {
        const float bw = std::min(230.0f, w * 0.26f);
        const float bh = 44.0f;
        const float gap = 16.0f;
        const float gridW = bw * 3.0f + gap * 2.0f;
        const float x0 = (w - gridW) * 0.5f;
        const float y0 = h * 0.22f;
        for (size_t i = 0; i < comicFiles_.size(); ++i) {
            const int col = static_cast<int>(i % 3);
            const int row = static_cast<int>(i / 3);
            SDL_FRect rect{x0 + col * (bw + gap), y0 + row * (bh + gap), bw, bh};
            if (pointIn(rect, x, y)) return static_cast<int>(i);
        }
        return -1;
    }

    void drawMosesArms(float x, float y, bool held)
    {
        filledCircle(x, y - 34.0f, 8.0f, {102, 61, 34});
        triangle({{x, y - 20.0f}, {x - 16.0f, y + 44.0f}, {x + 16.0f, y + 44.0f}}, {76, 42, 148});
        if (held) {
            line(x - 8.0f, y - 14.0f, x - 54.0f, y - 64.0f, kBrown);
            line(x + 8.0f, y - 14.0f, x + 54.0f, y - 64.0f, kBrown);
        } else {
            line(x - 8.0f, y - 14.0f, x - 58.0f, y + 18.0f, kBrown);
            line(x + 8.0f, y - 14.0f, x + 58.0f, y + 18.0f, kBrown);
        }
    }

    void drawFighter(float x, float y, bool leftSide, double t)
    {
        const float slash = 10.0f + 4.0f * std::sin(t * 9.0);
        filledCircle(x, y - 20.0f, 5.0f, {102, 61, 34});
        triangle({{x, y - 12.0f}, {x - 7.0f, y + 16.0f}, {x + 7.0f, y + 16.0f}},
                 leftSide ? kBlue : kRed);
        const float dir = leftSide ? 1.0f : -1.0f;
        line(x, y - 6.0f, x + dir * slash, y - 28.0f, kBlack);
        line(x, y + 16.0f, x - 6.0f, y + 28.0f, kBlack);
        line(x, y + 16.0f, x + 6.0f, y + 28.0f, kBlack);
    }

    void drawBird(double x, double y, double flap)
    {
        line(x - 12.0, y, x, y - flap, kBlack);
        line(x, y - flap, x + 12.0, y, kBlack);
        filledCircle(static_cast<float>(x), static_cast<float>(y - flap), 2.5f, kBlack);
    }

    void drawDeadBird(double x, double y)
    {
        line(x - 7.0, y - 6.0, x + 7.0, y + 6.0, kBlack);
        line(x + 7.0, y - 6.0, x - 7.0, y + 6.0, kBlack);
    }

    void drawComicArt(SDL_FRect panel, int index)
    {
        const float x = panel.x;
        const float y = panel.y;
        const float w = panel.w;
        const float h = panel.h;

        if (index >= 0 && static_cast<size_t>(index) < comicFiles_.size()) {
            const std::string &file = comicFiles_[static_cast<size_t>(index)];
            int bi = 1;
            if ((index == 1 || index == 5) && sprites_.has(file, 2) &&
                std::fmod(nowSeconds(), 0.8) >= 0.4) {
                bi = 2;
            }
            const float scale = (w - 36.0f) / 640.0f;
            if (sprites_.draw(file, bi, x + 18.0f, y + h * 0.66f, scale)) return;
        }
    }

    void drawHelp(int contentW, int h)
    {
        drawScrollableDoc("Help", textAssets_.helpLines(), contentW, h,
                          "Scroll with wheel/PageUp/PageDown/arrows. Esc returns.");
    }
};

struct Args {
    bool smokeWindow = false;
    bool smokeScenes = false;
};

Args parseArgs(int argc, char **argv)
{
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--smoke-window") {
            args.smokeWindow = true;
        } else if (arg == "--smoke-scenes") {
            args.smokeScenes = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: after-egypt-sdl3 [--smoke-window] [--smoke-scenes]\n";
            std::exit(0);
        }
    }
    return args;
}

} // namespace

int main(int argc, char **argv)
{
    try {
        const Args args = parseArgs(argc, argv);
        AfterEgyptApp app;
        if (args.smokeScenes) return app.smokeScenes();
        return app.run(args.smokeWindow ? 3 : 0);
    } catch (const std::exception &e) {
        std::cerr << "after-egypt-sdl3: " << e.what() << '\n';
        return 1;
    }
}
