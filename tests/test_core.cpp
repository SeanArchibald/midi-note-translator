#include "core.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {

int g_failures = 0;

void fail(const char* file, int line, const char* expression) {
    std::fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
    ++g_failures;
}

#define CHECK(expression)                                                      \
    do {                                                                       \
        if (!(expression))                                                     \
            fail(__FILE__, __LINE__, #expression);                             \
    } while (false)

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        const long long a = static_cast<long long>(actual);                    \
        const long long e = static_cast<long long>(expected);                  \
        if (a != e) {                                                          \
            std::fprintf(stderr, "%s:%d: expected %s == %lld, got %lld\n",     \
                         __FILE__, __LINE__, #actual, e, a);                   \
            ++g_failures;                                                      \
        }                                                                      \
    } while (false)

#define CHECK_NEAR(actual, expected, tolerance)                                \
    do {                                                                       \
        const double a = (actual);                                             \
        const double e = (expected);                                           \
        if (std::fabs(a - e) > (tolerance)) {                                  \
            std::fprintf(stderr, "%s:%d: expected %s ~= %f, got %f\n",         \
                         __FILE__, __LINE__, #actual, e, a);                   \
            ++g_failures;                                                      \
        }                                                                      \
    } while (false)

using midi_note_translator::NoteCore;

void testDefaults() {
    NoteCore core;

    CHECK_EQ(core.inputNoteCount(), 12);
    CHECK_EQ(core.outputNoteCount(), 12);
    CHECK_EQ(core.baseMidiNote(), 60);
    CHECK_NEAR(core.nudge(), 0.0, 0.0);

    CHECK_EQ(core.translateNote(60), 60);
    CHECK_EQ(core.translateNote(72), 72);
    CHECK_EQ(core.translateNote(48), 48);
    CHECK_EQ(core.translateNote(0), 0);
    CHECK_EQ(core.translateNote(127), 127);

    CHECK_EQ(core.translateNote(-5), 0);
    CHECK_EQ(core.translateNote(200), 127);
}

void test12To24() {
    NoteCore core;
    core.setOutputNoteCount(24);

    CHECK_EQ(core.translateNote(60), 60);
    CHECK_EQ(core.translateNote(72), 84);
    CHECK_EQ(core.translateNote(48), 36);
    CHECK_EQ(core.translateNote(84), 108);
    CHECK_EQ(core.translateNote(90), 120);
    CHECK_EQ(core.translateNote(96), 127);
}

void test24To12() {
    NoteCore core;
    core.setInputNoteCount(24);
    core.setOutputNoteCount(12);

    CHECK_EQ(core.translateNote(60), 60);
    CHECK_EQ(core.translateNote(72), 66);
    CHECK_EQ(core.translateNote(84), 72);
    CHECK_EQ(core.translateNote(48), 54);
}

void testBaseNote() {
    NoteCore core;
    core.setBaseMidiNote(69);
    core.setOutputNoteCount(24);

    CHECK_EQ(core.translateNote(69), 69);
    CHECK_EQ(core.translateNote(81), 93);
    CHECK_EQ(core.translateNote(57), 45);
}

void testNudge() {
    NoteCore core;
    core.setNudge(0.5);

    CHECK_EQ(core.translateNote(60), 61);
    CHECK_EQ(core.translateNote(61), 62);
    CHECK_EQ(core.translateNote(127), 127);

    core.setNudge(0.49);
    CHECK_EQ(core.translateNote(60), 60);
    CHECK_EQ(core.translateNote(61), 61);

    core.setNudge(-0.5);
    CHECK_EQ(core.translateNote(60), 60);
    CHECK_EQ(core.translateNote(61), 61);
}

void testParameterClamping() {
    NoteCore core;

    core.setInputNoteCount(0.0);
    CHECK_EQ(core.inputNoteCount(), 1);

    core.setInputNoteCount(1000.0);
    CHECK_EQ(core.inputNoteCount(), 128);

    core.setOutputNoteCount(-4.0);
    CHECK_EQ(core.outputNoteCount(), 1);

    core.setBaseMidiNote(-1.0);
    CHECK_EQ(core.baseMidiNote(), 0);

    core.setBaseMidiNote(200.0);
    CHECK_EQ(core.baseMidiNote(), 127);

    core.setNudge(5.0);
    CHECK_NEAR(core.nudge(), 0.5, 0.0);

    core.setNudge(-5.0);
    CHECK_NEAR(core.nudge(), -0.5, 0.0);

    core.setNudge(0.25);
    core.setNudge(std::numeric_limits<double>::quiet_NaN());
    CHECK_NEAR(core.nudge(), 0.25, 0.0);
}

void testNotePairingAcrossParameterChange() {
    NoteCore core;

    CHECK_EQ(core.noteOn(-1, 0, 0, 60), 60);
    CHECK_EQ(core.noteOn(-1, 0, 0, 64), 64);
    CHECK_EQ(core.activeNoteCount(), 2);

    // Changing the output scale while notes are held must not change the key
    // used to release them.
    core.setOutputNoteCount(24);

    CHECK_EQ(core.noteOff(-1, 0, 0, 60), 60);
    CHECK_EQ(core.noteOff(-1, 0, 0, 64), 64);
    CHECK_EQ(core.activeNoteCount(), 0);

    // An untracked note-off falls back to translating the key.
    CHECK_EQ(core.noteOff(-1, 0, 0, 72), 84);
}

void testNoteIds() {
    NoteCore core;
    core.setOutputNoteCount(24);

    CHECK_EQ(core.noteOn(7, 0, 0, 61), 62);
    CHECK_EQ(core.activeNoteCount(), 1);

    core.setOutputNoteCount(12);

    // Same key, different note id: the note-on is tracked separately.
    CHECK_EQ(core.noteOn(9, 0, 0, 61), 61);
    CHECK_EQ(core.activeNoteCount(), 2);

    CHECK_EQ(core.noteOff(7, 0, 0, 61), 62);
    CHECK_EQ(core.noteOff(9, 0, 0, 61), 61);
    CHECK_EQ(core.activeNoteCount(), 0);

    // Hosts may provide an id on note-on but omit it on note-off.
    CHECK_EQ(core.noteOn(11, 0, 0, 61), 61);
    CHECK_EQ(core.noteOff(-1, 0, 0, 61), 61);
    CHECK_EQ(core.activeNoteCount(), 0);
}

void testWildcardNoteOff() {
    NoteCore core;

    CHECK_EQ(core.noteOn(-1, 0, 0, 60), 60);
    CHECK_EQ(core.noteOn(-1, 0, 0, 64), 64);
    CHECK_EQ(core.noteOn(-1, 0, 1, 67), 67);
    CHECK_EQ(core.activeNoteCount(), 3);

    // Release channel 0 on port 0 (key is a wildcard).
    CHECK_EQ(core.noteOff(-1, 0, 0, -1), -1);
    CHECK_EQ(core.activeNoteCount(), 1);

    // Release everything (all fields are wildcards).
    CHECK_EQ(core.noteOff(-1, -1, -1, -1), -1);
    CHECK_EQ(core.activeNoteCount(), 0);
}

void testExpressionLookup() {
    NoteCore core;
    core.setOutputNoteCount(24);

    CHECK_EQ(core.noteOn(-1, 0, 0, 60), 60);
    CHECK_EQ(core.noteOn(-1, 0, 0, 72), 84);

    CHECK_EQ(core.lookupOutputKey(-1, 0, 0, 72), 84);
    CHECK_EQ(core.lookupOutputKey(-1, 0, 0, 60), 60);

    // Untracked notes still get a translated key.
    CHECK_EQ(core.lookupOutputKey(-1, 0, 0, 74), 88);

    // Wildcard keys stay wildcards.
    CHECK_EQ(core.lookupOutputKey(-1, 0, 0, -1), -1);
}

void testReset() {
    NoteCore core;
    (void)core.noteOn(-1, 0, 0, 60);
    CHECK_EQ(core.activeNoteCount(), 1);

    core.reset();
    CHECK_EQ(core.activeNoteCount(), 0);
}

void testStateRoundTrip() {
    NoteCore source;
    source.setInputNoteCount(19.0);
    source.setOutputNoteCount(31.0);
    source.setBaseMidiNote(57.0);
    source.setNudge(0.25);

    std::uint8_t buffer[NoteCore::kStateSize] = {};
    source.saveState(buffer);

    NoteCore destination;
    CHECK(destination.loadState(buffer, sizeof(buffer)));

    CHECK_EQ(destination.inputNoteCount(), 19);
    CHECK_EQ(destination.outputNoteCount(), 31);
    CHECK_EQ(destination.baseMidiNote(), 57);
    CHECK_NEAR(destination.nudge(), 0.25, 0.0);

    // Loading a state must not leave stale active notes behind.
    (void)destination.noteOn(-1, 0, 0, 60);
    CHECK_EQ(destination.activeNoteCount(), 1);
    CHECK(destination.loadState(buffer, sizeof(buffer)));
    CHECK_EQ(destination.activeNoteCount(), 0);
}

void testStateValidation() {
    NoteCore source;
    source.setInputNoteCount(19.0);

    std::uint8_t buffer[NoteCore::kStateSize] = {};
    source.saveState(buffer);

    NoteCore target;

    std::uint8_t bad[NoteCore::kStateSize] = {};
    std::memcpy(bad, buffer, sizeof(bad));
    bad[0] = 'X';
    CHECK(!target.loadState(bad, sizeof(bad)));
    CHECK_EQ(target.inputNoteCount(), 12);

    CHECK(!target.loadState(buffer, sizeof(buffer) - 1));
    CHECK_EQ(target.inputNoteCount(), 12);

    std::memcpy(bad, buffer, sizeof(bad));
    bad[4] = 2; // bump the little-endian version word from 1 to 2
    CHECK(!target.loadState(bad, sizeof(bad)));
    CHECK_EQ(target.inputNoteCount(), 12);
}

} // namespace

int main() {
    testDefaults();
    test12To24();
    test24To12();
    testBaseNote();
    testNudge();
    testParameterClamping();
    testNotePairingAcrossParameterChange();
    testNoteIds();
    testWildcardNoteOff();
    testExpressionLookup();
    testReset();
    testStateRoundTrip();
    testStateValidation();

    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }

    std::printf("All NoteCore tests passed.\n");
    return 0;
}
