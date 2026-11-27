#pragma once

#include "core.h"

#include <clap/clap.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>

#include <cstdint>

namespace midi_note_translator {

enum class ParamId : uint32_t {
    InputNoteCount = 0,
    OutputNoteCount = 1,
    BaseMidiNote = 2,
    Nudge = 3,
    Count = 4
};

class Plugin : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal> {
public:
    explicit Plugin(const clap_host* host);

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

#ifdef MNT_VST3_SILENT_OUTPUT
    // clap_plugin_audio_ports: the VST3 build presents a silent stereo output
    // so hosts load it as a normal instrument.
    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info* info) const noexcept override;
#endif

    // clap_plugin_state
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream* stream) noexcept override;
    bool stateLoad(const clap_istream* stream) noexcept override;

protected:
    void handleParamEvent(const clap_event_param_value* event) noexcept;
    void processMidiEvent(clap_event_midi& event) noexcept;

private:
    NoteCore m_core;
};

} // namespace midi_note_translator
