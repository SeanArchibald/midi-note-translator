#include "plugin.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace midi_note_translator {

static const char* const PLUGIN_NAME = "MIDI Note Translator";
static const char* const PLUGIN_VENDOR = "Sevish";
static const char* const PLUGIN_URL = "";
static const char* const PLUGIN_MANUAL_URL = "";
static const char* const PLUGIN_SUPPORT_URL = "";
static const char* const PLUGIN_VERSION = "1.0.0";
static const char* const PLUGIN_DESCRIPTION = "Translates MIDI note numbers between equal divisions";

// Plugin features - note effect that handles MIDI
static const char* s_features[] = {
    CLAP_PLUGIN_FEATURE_NOTE_EFFECT,
    CLAP_PLUGIN_FEATURE_UTILITY,
    nullptr
};

// Plugin descriptor - properly initialized with features at compile time
static const clap_plugin_descriptor s_descriptor = {
    CLAP_VERSION,
    "com.sevish.midi-note-translator",
    PLUGIN_NAME,
    PLUGIN_VENDOR,
    PLUGIN_URL,
    PLUGIN_MANUAL_URL,
    PLUGIN_SUPPORT_URL,
    PLUGIN_VERSION,
    PLUGIN_DESCRIPTION,
    s_features,
};

const clap_plugin_descriptor* Plugin::descriptor() {
    return &s_descriptor;
}

Plugin::Plugin(const clap_host* host)
    : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>(descriptor(), host) {
}

bool Plugin::init() noexcept {
    if (!clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>::init())
        return false;
    return true;
}

bool Plugin::activate(double sampleRate, uint32_t minFramesCount, uint32_t maxFramesCount) noexcept {
    (void)sampleRate;
    (void)minFramesCount;
    (void)maxFramesCount;
    return true;
}

void Plugin::deactivate() noexcept {
}

bool Plugin::startProcessing() noexcept {
    return true;
}

void Plugin::stopProcessing() noexcept {
}

void Plugin::reset() noexcept {
    m_core.reset();
}

bool Plugin::stateSave(const clap_ostream* stream) noexcept {
    uint8_t buffer[NoteCore::kStateSize] = {};
    m_core.saveState(buffer);

    // Streams may perform partial writes; keep going until the state is out.
    size_t offset = 0;
    while (offset < sizeof(buffer)) {
        const int64_t written = stream->write(stream, buffer + offset, sizeof(buffer) - offset);
        if (written <= 0)
            return false;
        offset += static_cast<size_t>(written);
    }
    return true;
}

bool Plugin::stateLoad(const clap_istream* stream) noexcept {
    uint8_t buffer[NoteCore::kStateSize] = {};

    // Reads may also be partial.
    size_t offset = 0;
    while (offset < sizeof(buffer)) {
        const int64_t read = stream->read(stream, buffer + offset, sizeof(buffer) - offset);
        if (read <= 0)
            return false;
        offset += static_cast<size_t>(read);
    }

    if (!m_core.loadState(buffer, sizeof(buffer)))
        return false;

    // Loading state changes host-visible parameter values; tell the host.
    const clap_host_params* hostParams = nullptr;
    _host.getExtension(hostParams, CLAP_EXT_PARAMS);
    if (hostParams != nullptr && hostParams->rescan != nullptr)
        hostParams->rescan(_host.host(), CLAP_PARAM_RESCAN_VALUES);

    return true;
}

bool Plugin::paramsInfo(uint32_t paramIndex, clap_param_info* info) const noexcept {
    if (paramIndex >= static_cast<uint32_t>(ParamId::Count))
        return false;

    info->id = paramIndex;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;

    switch (static_cast<ParamId>(paramIndex)) {
        case ParamId::InputNoteCount:
            strncpy(info->name, "Input Note Count", sizeof(info->name));
            strncpy(info->module, "Scale", sizeof(info->module));
            info->min_value = 1;
            info->max_value = 128;
            info->default_value = 12;
            break;
        case ParamId::OutputNoteCount:
            strncpy(info->name, "Output Note Count", sizeof(info->name));
            strncpy(info->module, "Scale", sizeof(info->module));
            info->min_value = 1;
            info->max_value = 128;
            info->default_value = 12;
            break;
        case ParamId::BaseMidiNote:
            strncpy(info->name, "Base MIDI Note", sizeof(info->name));
            strncpy(info->module, "Scale", sizeof(info->module));
            info->min_value = 0;
            info->max_value = 127;
            info->default_value = 60;
            break;
        case ParamId::Nudge:
            strncpy(info->name, "Nudge", sizeof(info->name));
            strncpy(info->module, "Scale", sizeof(info->module));
            info->min_value = -0.5;
            info->max_value = 0.5;
            info->default_value = 0.0;
            info->flags &= ~CLAP_PARAM_IS_STEPPED; // Float parameter
            break;
        default:
            return false;
    }

    return true;
}

bool Plugin::paramsValue(clap_id paramId, double* value) noexcept {
    switch (static_cast<ParamId>(paramId)) {
        case ParamId::InputNoteCount:
            *value = static_cast<double>(m_core.inputNoteCount());
            return true;
        case ParamId::OutputNoteCount:
            *value = static_cast<double>(m_core.outputNoteCount());
            return true;
        case ParamId::BaseMidiNote:
            *value = static_cast<double>(m_core.baseMidiNote());
            return true;
        case ParamId::Nudge:
            *value = m_core.nudge();
            return true;
        default:
            return false;
    }
}

bool Plugin::paramsValueToText(clap_id paramId, double value, char* display, uint32_t size) noexcept {
    if (size == 0)
        return false;

    switch (static_cast<ParamId>(paramId)) {
        case ParamId::InputNoteCount:
        case ParamId::OutputNoteCount:
        case ParamId::BaseMidiNote:
            snprintf(display, size, "%d", static_cast<int>(value));
            return true;
        case ParamId::Nudge:
            snprintf(display, size, "%.3f", value);
            return true;
        default:
            return false;
    }
}

bool Plugin::paramsTextToValue(clap_id paramId, const char* display, double* value) noexcept {
    char* endptr = nullptr;
    double v = std::strtod(display, &endptr);
    if (endptr == display || *endptr != '\0')
        return false;

    switch (static_cast<ParamId>(paramId)) {
        case ParamId::InputNoteCount:
        case ParamId::OutputNoteCount:
        case ParamId::BaseMidiNote:
            *value = std::round(v);
            return true;
        case ParamId::Nudge:
            *value = v;
            return true;
        default:
            return false;
    }
}

void Plugin::paramsFlush(const clap_input_events* in, const clap_output_events* out) noexcept {
    (void)out;

    const uint32_t eventCount = in->size(in);
    for (uint32_t i = 0; i < eventCount; ++i) {
        const clap_event_header* header = in->get(in, i);
        if (header == nullptr || header->space_id != CLAP_CORE_EVENT_SPACE_ID)
            continue;
        if (header->type != CLAP_EVENT_PARAM_VALUE ||
            header->size < sizeof(clap_event_param_value))
            continue;

        handleParamEvent(reinterpret_cast<const clap_event_param_value*>(header));
    }
}

uint32_t Plugin::notePortsCount(bool isInput) const noexcept {
    (void)isInput;
    return 1; // One note input port and one note output port
}

bool Plugin::notePortsInfo(uint32_t index, bool isInput, clap_note_port_info* info) const noexcept {
    if (index != 0)
        return false;

    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    strncpy(info->name, isInput ? "Note Input" : "Note Output", sizeof(info->name));
    return true;
}

void Plugin::handleParamEvent(const clap_event_param_value* event) noexcept {
    switch (static_cast<ParamId>(event->param_id)) {
        case ParamId::InputNoteCount:
            m_core.setInputNoteCount(event->value);
            break;
        case ParamId::OutputNoteCount:
            m_core.setOutputNoteCount(event->value);
            break;
        case ParamId::BaseMidiNote:
            m_core.setBaseMidiNote(event->value);
            break;
        case ParamId::Nudge:
            m_core.setNudge(event->value);
            break;
        default:
            break;
    }
}

void Plugin::processMidiEvent(clap_event_midi& event) noexcept {
    const uint8_t status = event.data[0] & 0xf0;
    if (status != 0x90 && status != 0x80)
        return;

    const auto channel = static_cast<int16_t>(event.data[0] & 0x0f);
    const auto key = static_cast<int16_t>(event.data[1]);
    const auto port = static_cast<int16_t>(event.port_index);
    const bool isNoteOn = status == 0x90 && event.data[2] > 0;

    if (isNoteOn)
        event.data[1] = static_cast<uint8_t>(m_core.noteOn(-1, port, channel, key));
    else
        event.data[1] = static_cast<uint8_t>(m_core.noteOff(-1, port, channel, key));
}

clap_process_status Plugin::process(const clap_process* process) noexcept {
    if (process->in_events == nullptr || process->out_events == nullptr)
        return CLAP_PROCESS_CONTINUE;

    const clap_output_events* out = process->out_events;
    const uint32_t eventCount = process->in_events->size(process->in_events);

    for (uint32_t i = 0; i < eventCount; ++i) {
        const clap_event_header* header = process->in_events->get(process->in_events, i);
        if (header == nullptr)
            continue;

        // Only core events are interpreted; everything else is passed through.
        if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
            out->try_push(out, header);
            continue;
        }

        switch (header->type) {
            case CLAP_EVENT_PARAM_VALUE: {
                if (header->size >= sizeof(clap_event_param_value))
                    handleParamEvent(reinterpret_cast<const clap_event_param_value*>(header));
                out->try_push(out, header);
                break;
            }
            case CLAP_EVENT_NOTE_ON: {
                if (header->size < sizeof(clap_event_note)) {
                    out->try_push(out, header);
                    break;
                }

                clap_event_note event{};
                std::memcpy(&event, header, sizeof(event));

                // Note-on addresses are required to be concrete.
                if (event.key < 0 || event.channel < 0 || event.port_index < 0) {
                    out->try_push(out, &event.header);
                    break;
                }

                event.key = m_core.noteOn(event.note_id, event.port_index,
                                          event.channel, event.key);
                out->try_push(out, &event.header);
                break;
            }
            case CLAP_EVENT_NOTE_OFF: {
                if (header->size < sizeof(clap_event_note)) {
                    out->try_push(out, header);
                    break;
                }

                clap_event_note event{};
                std::memcpy(&event, header, sizeof(event));

                // Keep wildcard note-offs wildcard: all downstream voices came
                // from this plugin, so the same wildcard is still correct.
                event.key = m_core.noteOff(event.note_id, event.port_index,
                                           event.channel, event.key);
                out->try_push(out, &event.header);
                break;
            }
            case CLAP_EVENT_NOTE_CHOKE:
            case CLAP_EVENT_NOTE_END: {
                if (header->size < sizeof(clap_event_note)) {
                    out->try_push(out, header);
                    break;
                }

                clap_event_note event{};
                std::memcpy(&event, header, sizeof(event));
                if (event.key >= 0)
                    event.key = m_core.lookupOutputKey(event.note_id, event.port_index,
                                                       event.channel, event.key);
                out->try_push(out, &event.header);
                break;
            }
            case CLAP_EVENT_NOTE_EXPRESSION: {
                if (header->size < sizeof(clap_event_note_expression)) {
                    out->try_push(out, header);
                    break;
                }

                clap_event_note_expression event{};
                std::memcpy(&event, header, sizeof(event));
                if (event.key >= 0)
                    event.key = m_core.lookupOutputKey(event.note_id, event.port_index,
                                                       event.channel, event.key);
                out->try_push(out, &event.header);
                break;
            }
            case CLAP_EVENT_MIDI: {
                if (header->size < sizeof(clap_event_midi)) {
                    out->try_push(out, header);
                    break;
                }

                clap_event_midi event{};
                std::memcpy(&event, header, sizeof(event));
                processMidiEvent(event);
                out->try_push(out, &event.header);
                break;
            }
            case CLAP_EVENT_TRANSPORT:
                // Transport is host-to-plugin only; emitting it is invalid.
                break;
            default:
                out->try_push(out, header);
                break;
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

} // namespace midi_note_translator
