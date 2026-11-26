#include "core.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace midi_note_translator {

namespace {

constexpr std::uint32_t kStateVersion = 1;
constexpr char kStateMagic[4] = {'M', 'N', 'T', '1'};

static_assert(std::atomic<std::int32_t>::is_always_lock_free,
              "parameter atomics must be lock-free for realtime safety");
static_assert(std::atomic<double>::is_always_lock_free,
              "parameter atomics must be lock-free for realtime safety");

void writeU32(std::uint8_t*& cursor, std::uint32_t value) noexcept {
    cursor[0] = static_cast<std::uint8_t>(value & 0xffu);
    cursor[1] = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    cursor[2] = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    cursor[3] = static_cast<std::uint8_t>((value >> 24) & 0xffu);
    cursor += 4;
}

std::uint32_t readU32(const std::uint8_t*& cursor) noexcept {
    const std::uint32_t value = static_cast<std::uint32_t>(cursor[0]) |
                                (static_cast<std::uint32_t>(cursor[1]) << 8) |
                                (static_cast<std::uint32_t>(cursor[2]) << 16) |
                                (static_cast<std::uint32_t>(cursor[3]) << 24);
    cursor += 4;
    return value;
}

void writeU64(std::uint8_t*& cursor, std::uint64_t value) noexcept {
    for (int i = 0; i < 8; ++i)
        cursor[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xffu);
    cursor += 8;
}

std::uint64_t readU64(const std::uint8_t*& cursor) noexcept {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(cursor[i]) << (8 * i);
    cursor += 8;
    return value;
}

void writeDouble(std::uint8_t*& cursor, double value) noexcept {
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    writeU64(cursor, bits);
}

double readDouble(const std::uint8_t*& cursor) noexcept {
    const std::uint64_t bits = readU64(cursor);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

} // namespace

void NoteCore::reset() noexcept {
    for (auto& note : m_activeNotes)
        note = ActiveNote{};
}

void NoteCore::setInputNoteCount(double value) noexcept {
    if (!std::isfinite(value))
        return;
    const auto count = static_cast<std::int32_t>(std::lround(std::clamp(value, 1.0, 128.0)));
    m_inputNoteCount.store(count, std::memory_order_relaxed);
}

void NoteCore::setOutputNoteCount(double value) noexcept {
    if (!std::isfinite(value))
        return;
    const auto count = static_cast<std::int32_t>(std::lround(std::clamp(value, 1.0, 128.0)));
    m_outputNoteCount.store(count, std::memory_order_relaxed);
}

void NoteCore::setBaseMidiNote(double value) noexcept {
    if (!std::isfinite(value))
        return;
    const auto note = static_cast<std::int32_t>(std::lround(std::clamp(value, 0.0, 127.0)));
    m_baseMidiNote.store(note, std::memory_order_relaxed);
}

void NoteCore::setNudge(double value) noexcept {
    if (!std::isfinite(value))
        return;
    m_nudge.store(std::clamp(value, -0.5, 0.5), std::memory_order_relaxed);
}

std::int32_t NoteCore::inputNoteCount() const noexcept {
    return m_inputNoteCount.load(std::memory_order_relaxed);
}

std::int32_t NoteCore::outputNoteCount() const noexcept {
    return m_outputNoteCount.load(std::memory_order_relaxed);
}

std::int32_t NoteCore::baseMidiNote() const noexcept {
    return m_baseMidiNote.load(std::memory_order_relaxed);
}

double NoteCore::nudge() const noexcept {
    return m_nudge.load(std::memory_order_relaxed);
}

std::int32_t NoteCore::translateNote(std::int32_t inputNote) const noexcept {
    const std::int32_t input = std::clamp(inputNote, 0, 127);

    const double inputCount = static_cast<double>(inputNoteCount());
    const double outputCount = static_cast<double>(outputNoteCount());
    const double baseNote = static_cast<double>(baseMidiNote());

    const double ratio = outputCount / inputCount;
    const double delta = (static_cast<double>(input) - baseNote) * ratio;
    const double result = baseNote + nudge() + delta;

    const auto output = static_cast<std::int32_t>(std::round(result));
    return std::clamp(output, 0, 127);
}

bool NoteCore::matches(const ActiveNote& note, std::int32_t noteId,
                       std::int16_t portIndex, std::int16_t channel,
                       std::int16_t key) noexcept {
    return note.active && (noteId < 0 || note.noteId == noteId) &&
           (portIndex < 0 || note.portIndex == portIndex) &&
           (channel < 0 || note.channel == channel) &&
           (key < 0 || note.inputKey == key);
}

NoteCore::ActiveNote* NoteCore::findForNoteOn(std::int32_t noteId,
                                              std::int16_t portIndex,
                                              std::int16_t channel,
                                              std::int16_t key) noexcept {
    for (auto& note : m_activeNotes) {
        if (!note.active)
            continue;

        if (noteId >= 0) {
            if (note.noteId == noteId && note.portIndex == portIndex &&
                note.channel == channel)
                return &note;
        } else if (note.noteId < 0 && note.inputKey == key &&
                   note.portIndex == portIndex && note.channel == channel) {
            return &note;
        }
    }
    return nullptr;
}

const NoteCore::ActiveNote* NoteCore::findForNoteOff(std::int32_t noteId,
                                                     std::int16_t portIndex,
                                                     std::int16_t channel,
                                                     std::int16_t key) const noexcept {
    if (noteId >= 0) {
        for (const auto& note : m_activeNotes) {
            if (matches(note, noteId, portIndex, channel, key))
                return &note;
        }
    }

    // A host may provide a note id on note-on but not on note-off (or use
    // another wildcard), so fall back to matching the full address without it.
    for (const auto& note : m_activeNotes) {
        if (matches(note, -1, portIndex, channel, key))
            return &note;
    }
    return nullptr;
}

std::int16_t NoteCore::noteOn(std::int32_t noteId, std::int16_t portIndex,
                              std::int16_t channel, std::int16_t key) noexcept {
    const auto outputKey = static_cast<std::int16_t>(translateNote(key));

    if (auto* existing = findForNoteOn(noteId, portIndex, channel, key)) {
        existing->outputKey = outputKey;
        existing->active = true;
        return outputKey;
    }

    for (auto& note : m_activeNotes) {
        if (!note.active) {
            note.noteId = noteId;
            note.portIndex = portIndex;
            note.channel = channel;
            note.inputKey = key;
            note.outputKey = outputKey;
            note.active = true;
            return outputKey;
        }
    }

    // The table is full (pathological host behaviour): play the note but leave
    // it untracked, so its note-off falls back to translating the key.
    return outputKey;
}

std::int16_t NoteCore::noteOff(std::int32_t noteId, std::int16_t portIndex,
                               std::int16_t channel, std::int16_t key) noexcept {
    if (key < 0) {
        for (auto& note : m_activeNotes) {
            if (matches(note, noteId, portIndex, channel, -1))
                note.active = false;
        }
        return -1;
    }

    const ActiveNote* found = findForNoteOff(noteId, portIndex, channel, key);
    if (found == nullptr)
        return static_cast<std::int16_t>(translateNote(key));

    const std::int16_t outputKey = found->outputKey;
    const_cast<ActiveNote*>(found)->active = false;
    return outputKey;
}

std::int16_t NoteCore::lookupOutputKey(std::int32_t noteId, std::int16_t portIndex,
                                       std::int16_t channel,
                                       std::int16_t key) const noexcept {
    if (key < 0)
        return -1;

    const ActiveNote* found = findForNoteOff(noteId, portIndex, channel, key);
    if (found != nullptr)
        return found->outputKey;

    return static_cast<std::int16_t>(translateNote(key));
}

std::size_t NoteCore::activeNoteCount() const noexcept {
    std::size_t count = 0;
    for (const auto& note : m_activeNotes) {
        if (note.active)
            ++count;
    }
    return count;
}

void NoteCore::saveState(std::uint8_t* out) const noexcept {
    std::uint8_t* cursor = out;
    std::memcpy(cursor, kStateMagic, sizeof(kStateMagic));
    cursor += sizeof(kStateMagic);
    writeU32(cursor, kStateVersion);
    writeU32(cursor, static_cast<std::uint32_t>(inputNoteCount()));
    writeU32(cursor, static_cast<std::uint32_t>(outputNoteCount()));
    writeU32(cursor, static_cast<std::uint32_t>(baseMidiNote()));
    writeDouble(cursor, nudge());
}

bool NoteCore::loadState(const std::uint8_t* data, std::size_t size) noexcept {
    if (data == nullptr || size < kStateSize)
        return false;

    const std::uint8_t* cursor = data;
    if (std::memcmp(cursor, kStateMagic, sizeof(kStateMagic)) != 0)
        return false;
    cursor += sizeof(kStateMagic);

    if (readU32(cursor) != kStateVersion)
        return false;

    const std::uint32_t input = readU32(cursor);
    const std::uint32_t output = readU32(cursor);
    const std::uint32_t base = readU32(cursor);
    const double nudgeValue = readDouble(cursor);

    setInputNoteCount(static_cast<double>(input));
    setOutputNoteCount(static_cast<double>(output));
    setBaseMidiNote(static_cast<double>(base));
    setNudge(nudgeValue);
    reset();
    return true;
}

} // namespace midi_note_translator
