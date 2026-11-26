#pragma once

#include <clap/clap.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>
#include <array>
#include <cmath>
#include <unordered_map>

namespace midi_note_translator {

enum class ParamId : uint32_t {
    InputNoteCount = 0,
    OutputNoteCount = 1,
    BaseMidiNote = 2,
    Nudge = 3,
    Count = 4
};

// Key for tracking active notes (input note + channel + port)
struct ActiveNoteKey {
    int16_t key;
    int16_t channel;
    int16_t port_index;
    
    bool operator==(const ActiveNoteKey& other) const noexcept {
        return key == other.key && channel == other.channel && port_index == other.port_index;
    }
};

struct ActiveNoteKeyHash {
    size_t operator()(const ActiveNoteKey& k) const noexcept {
        return std::hash<int16_t>()(k.key) ^ (std::hash<int16_t>()(k.channel) << 1) ^ (std::hash<int16_t>()(k.port_index) << 2);
    }
};

// Information about an active note
struct ActiveNoteInfo {
    int16_t outputKey;
    int16_t channel;
    int16_t port_index;
    double velocity;  // Store original velocity for potential note-off
};

class Plugin : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal> {
public:
    Plugin(const clap_host* host);

    // clap_plugin_descriptor
    static const clap_plugin_descriptor* descriptor();

    // clap_plugin
    bool init() noexcept override;
    bool activate(double sampleRate, uint32_t minFramesCount, uint32_t maxFramesCount) noexcept override;
    void deactivate() noexcept override;
    bool startProcessing() noexcept override;
    void stopProcessing() noexcept override;
    void reset() noexcept override;
    clap_process_status process(const clap_process* process) noexcept override;

    // clap_plugin_params
    bool implementsParams() const noexcept override { return true; }
    uint32_t paramsCount() const noexcept override { return static_cast<uint32_t>(ParamId::Count); }
    bool paramsInfo(uint32_t paramIndex, clap_param_info* info) const noexcept override;
    bool paramsValue(clap_id paramId, double* value) noexcept override;
    bool paramsValueToText(clap_id paramId, double value, char* display, uint32_t size) noexcept override;
    bool paramsTextToValue(clap_id paramId, const char* display, double* value) noexcept override;
    void paramsFlush(const clap_input_events* in, const clap_output_events* out) noexcept override;

    // clap_plugin_note_ports
    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override;
    bool notePortsInfo(uint32_t index, bool isInput, clap_note_port_info* info) const noexcept override;

    // clap_plugin_state
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream* stream) noexcept override;
    bool stateLoad(const clap_istream* stream) noexcept override;

protected:
    void handleEvent(const clap_event_header* header, clap_output_events* out = nullptr);
    void processNoteOn(clap_event_note* event, clap_output_events* out);
    void processNoteOff(clap_event_note* event, clap_output_events* out);
    void processMidiEvent(clap_event_midi* event, clap_output_events* out);
    int32_t scaleNote(int32_t inputNote);
    void releaseAllActiveNotes(clap_output_events* out, uint32_t time);
    void sendNoteOff(clap_output_events* out, uint32_t time, const ActiveNoteKey& key, const ActiveNoteInfo& info);

private:
    // Parameter values
    int32_t m_inputNoteCount = 12;
    int32_t m_outputNoteCount = 12;
    int32_t m_baseMidiNote = 60;
    double m_nudge = 0.0;
    
    // Track active notes to handle stuck notes when parameters change
    std::unordered_map<ActiveNoteKey, ActiveNoteInfo, ActiveNoteKeyHash> m_activeNotes;
};

} // namespace midi_note_translator