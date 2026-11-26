#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace midi_note_translator {

// Format-independent note translation and active-note tracking.
//
// Parameter values are atomic because hosts read them from the main/UI thread
// while process() writes them from the audio thread. The active-note table is
// only touched from the audio thread (and when processing is stopped), so it
// needs no locking and never allocates.
class NoteCore {
public:
    static constexpr std::size_t kStateSize = 28;
    static constexpr std::size_t kMaxActiveNotes = 1024;

    NoteCore() noexcept = default;

    void reset() noexcept;

    void setInputNoteCount(double value) noexcept;
    void setOutputNoteCount(double value) noexcept;
    void setBaseMidiNote(double value) noexcept;
    void setNudge(double value) noexcept;

    [[nodiscard]] std::int32_t inputNoteCount() const noexcept;
    [[nodiscard]] std::int32_t outputNoteCount() const noexcept;
    [[nodiscard]] std::int32_t baseMidiNote() const noexcept;
    [[nodiscard]] double nudge() const noexcept;

    // Translates a note using the current parameters.
    [[nodiscard]] std::int32_t translateNote(std::int32_t inputNote) const noexcept;

    // Records a note-on and returns the translated key. Pass noteId = -1 when
    // the host does not provide one.
    [[nodiscard]] std::int16_t noteOn(std::int32_t noteId, std::int16_t portIndex,
                                      std::int16_t channel, std::int16_t key) noexcept;

    // Releases the matching active note and returns the key it was started
    // with. If key < 0 the event is a wildcard (as defined by CLAP): all
    // matching notes are released and -1 is returned so the caller can forward
    // the wildcard unchanged.
    [[nodiscard]] std::int16_t noteOff(std::int32_t noteId, std::int16_t portIndex,
                                       std::int16_t channel, std::int16_t key) noexcept;

    // Finds the translated key for a note expression/choke/end event without
    // changing the active-note state. key < 0 returns -1 (wildcard).
    [[nodiscard]] std::int16_t lookupOutputKey(std::int32_t noteId, std::int16_t portIndex,
                                               std::int16_t channel,
                                               std::int16_t key) const noexcept;

    [[nodiscard]] std::size_t activeNoteCount() const noexcept;

    void saveState(std::uint8_t* out) const noexcept;
    [[nodiscard]] bool loadState(const std::uint8_t* data, std::size_t size) noexcept;

private:
    struct ActiveNote {
        std::int32_t noteId = -1;
        std::int16_t portIndex = -1;
        std::int16_t channel = -1;
        std::int16_t inputKey = -1;
        std::int16_t outputKey = -1;
        bool active = false;
    };

    static bool matches(const ActiveNote& note, std::int32_t noteId,
                        std::int16_t portIndex, std::int16_t channel,
                        std::int16_t key) noexcept;

    ActiveNote* findForNoteOn(std::int32_t noteId, std::int16_t portIndex,
                              std::int16_t channel, std::int16_t key) noexcept;

    [[nodiscard]] const ActiveNote* findForNoteOff(std::int32_t noteId,
                                                   std::int16_t portIndex,
                                                   std::int16_t channel,
                                                   std::int16_t key) const noexcept;

    std::array<ActiveNote, kMaxActiveNotes> m_activeNotes{};

    std::atomic<std::int32_t> m_inputNoteCount{12};
    std::atomic<std::int32_t> m_outputNoteCount{12};
    std::atomic<std::int32_t> m_baseMidiNote{60};
    std::atomic<double> m_nudge{0.0};
};

} // namespace midi_note_translator
