#include "plugin.h"
#include <cstring>
#include <string>
#include <cstdio>

namespace scale_scaler {

static const char* const PLUGIN_NAME = "Scale Scaler";
static const char* const PLUGIN_VENDOR = "Sevish";
static const char* const PLUGIN_URL = "";
static const char* const PLUGIN_MANUAL_URL = "";
static const char* const PLUGIN_SUPPORT_URL = "";
static const char* const PLUGIN_VERSION = "1.0.0";
static const char* const PLUGIN_DESCRIPTION = "Scales MIDI note numbers between different sized tunings";

// Plugin features - note effect that handles MIDI
static const char* s_features[] = {
    CLAP_PLUGIN_FEATURE_NOTE_EFFECT,
    CLAP_PLUGIN_FEATURE_UTILITY,
    nullptr
};

// Plugin descriptor - properly initialized with features at compile time
static const clap_plugin_descriptor s_descriptor = {
    CLAP_VERSION,
    "com.sevish.scale-scaler",
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
    m_activeNotes.clear();
}

bool Plugin::stateSave(const clap_ostream* stream) noexcept {
    // Write all parameter values to the stream
    // Format: inputNoteCount (int32), outputNoteCount (int32), baseMidiNote (int32), nudge (double)
    
    int32_t values[3] = { m_inputNoteCount, m_outputNoteCount, m_baseMidiNote };
    int64_t written = stream->write(stream, values, sizeof(values));
    if (written != sizeof(values))
        return false;
    
    written = stream->write(stream, &m_nudge, sizeof(m_nudge));
    if (written != sizeof(m_nudge))
        return false;
    
    return true;
}

bool Plugin::stateLoad(const clap_istream* stream) noexcept {
    // Read all parameter values from the stream
    int32_t values[3];
    int64_t read = stream->read(stream, values, sizeof(values));
    if (read != sizeof(values))
        return false;
    
    double nudge;
    read = stream->read(stream, &nudge, sizeof(nudge));
    if (read != sizeof(nudge))
        return false;
    
    // Validate and apply values
    m_inputNoteCount = std::max(1, std::min(128, values[0]));
    m_outputNoteCount = std::max(1, std::min(128, values[1]));
    m_baseMidiNote = std::max(0, std::min(127, values[2]));
    m_nudge = std::max(-0.5, std::min(0.5, nudge));
    
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
            *value = static_cast<double>(m_inputNoteCount);
            return true;
        case ParamId::OutputNoteCount:
            *value = static_cast<double>(m_outputNoteCount);
            return true;
        case ParamId::BaseMidiNote:
            *value = static_cast<double>(m_baseMidiNote);
            return true;
        case ParamId::Nudge:
            *value = m_nudge;
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
        handleEvent(header);
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

void Plugin::handleEvent(const clap_event_header* header, clap_output_events* out) {
    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (header->type) {
        case CLAP_EVENT_PARAM_VALUE: {
            auto ev = reinterpret_cast<const clap_event_param_value*>(header);
            
            switch (static_cast<ParamId>(ev->param_id)) {
                case ParamId::InputNoteCount:
                    m_inputNoteCount = static_cast<int32_t>(std::round(ev->value));
                    m_inputNoteCount = std::max(1, std::min(128, m_inputNoteCount));
                    break;
                case ParamId::OutputNoteCount:
                    m_outputNoteCount = static_cast<int32_t>(std::round(ev->value));
                    m_outputNoteCount = std::max(1, std::min(128, m_outputNoteCount));
                    break;
                case ParamId::BaseMidiNote:
                    m_baseMidiNote = static_cast<int32_t>(std::round(ev->value));
                    m_baseMidiNote = std::max(0, std::min(127, m_baseMidiNote));
                    break;
                case ParamId::Nudge:
                    m_nudge = ev->value;
                    m_nudge = std::max(-0.5, std::min(0.5, m_nudge));
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

int32_t Plugin::scaleNote(int32_t inputNote) {
    // Clamp input to valid MIDI range
    inputNote = std::max(0, std::min(127, inputNote));
    
    // Formula: output = round(base MIDI note + nudge + ((input - base MIDI note) * (output note count / input note count)))
    double inputNoteCount = static_cast<double>(m_inputNoteCount);
    double outputNoteCount = static_cast<double>(m_outputNoteCount);
    double baseNote = static_cast<double>(m_baseMidiNote);
    
    double ratio = outputNoteCount / inputNoteCount;
    double delta = (static_cast<double>(inputNote) - baseNote) * ratio;
    double result = baseNote + m_nudge + delta;
    
    // Round to integer
    int32_t outputNote = static_cast<int32_t>(std::round(result));
    
    // Clamp to valid MIDI range 0-127
    outputNote = std::max(0, std::min(127, outputNote));
    
    return outputNote;
}

void Plugin::sendNoteOff(clap_output_events* out, uint32_t time, const ActiveNoteKey& /*key*/, const ActiveNoteInfo& info) {
    clap_event_note noteOff;
    noteOff.header.size = sizeof(clap_event_note);
    noteOff.header.time = time;
    noteOff.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    noteOff.header.type = CLAP_EVENT_NOTE_OFF;
    noteOff.header.flags = 0;
    noteOff.port_index = info.port_index;
    noteOff.channel = info.channel;
    noteOff.key = info.outputKey;
    noteOff.note_id = -1;  // Not using note_id
    noteOff.velocity = 0.0;
    
    out->try_push(out, &noteOff.header);
}

void Plugin::releaseAllActiveNotes(clap_output_events* out, uint32_t time) {
    for (const auto& [key, info] : m_activeNotes) {
        sendNoteOff(out, time, key, info);
    }
    m_activeNotes.clear();
}

void Plugin::processNoteOn(clap_event_note* event, clap_output_events* out) {
    (void)out;
    int16_t originalKey = event->key;
    
    // Calculate the scaled note
    int16_t scaledKey = static_cast<int16_t>(scaleNote(event->key));
    event->key = scaledKey;
    
    // Track this active note (using original key as the lookup)
    ActiveNoteKey key{originalKey, event->channel, event->port_index};
    ActiveNoteInfo info{scaledKey, event->channel, event->port_index, event->velocity};
    m_activeNotes[key] = info;
}

void Plugin::processNoteOff(clap_event_note* event, clap_output_events* out) {
    (void)out;
    int16_t originalKey = event->key;
    
    // Look up the original output note from when this note was turned on
    ActiveNoteKey key{originalKey, event->channel, event->port_index};
    auto it = m_activeNotes.find(key);
    
    if (it != m_activeNotes.end()) {
        // Use the tracked output note to ensure we turn off the right note
        event->key = it->second.outputKey;
        m_activeNotes.erase(it);
    } else {
        // Note not tracked - fallback to scaling (may be from before our tracking started)
        event->key = scaleNote(event->key);
    }
}

void Plugin::processMidiEvent(clap_event_midi* event, clap_output_events* /*out*/) {
    uint8_t status = event->data[0] & 0xF0;
    uint8_t channel = event->data[0] & 0x0F;
    
    // Check if this is a note on or note off message
    if (status == 0x90 || status == 0x80) {
        uint8_t originalNote = event->data[1];
        
        // Create a key for tracking (MIDI events use port 0)
        ActiveNoteKey key{static_cast<int16_t>(originalNote), static_cast<int16_t>(channel), 0};
        
        if (status == 0x90 && event->data[2] > 0) {
            // Note On (with velocity > 0)
            int16_t scaledNote = static_cast<int16_t>(scaleNote(static_cast<int32_t>(event->data[1])));
            event->data[1] = static_cast<uint8_t>(scaledNote);
            
            // Track this active note
            ActiveNoteInfo info{scaledNote, static_cast<int16_t>(channel), 0, static_cast<double>(event->data[2]) / 127.0};
            m_activeNotes[key] = info;
        } else {
            // Note Off (status 0x80 or velocity 0)
            auto it = m_activeNotes.find(key);
            if (it != m_activeNotes.end()) {
                // Use tracked output note
                event->data[1] = static_cast<uint8_t>(it->second.outputKey);
                m_activeNotes.erase(it);
            } else {
                // Untracked - fallback
                int32_t scaledNote = scaleNote(static_cast<int32_t>(event->data[1]));
                event->data[1] = static_cast<uint8_t>(scaledNote);
            }
        }
    }
}

clap_process_status Plugin::process(const clap_process* process) noexcept {
    (void)process->frames_count;
    const uint32_t eventCount = process->in_events->size(process->in_events);
    
    for (uint32_t i = 0; i < eventCount; ++i) {
        const clap_event_header* header = process->in_events->get(process->in_events, i);
        
        // Handle parameter changes (pass output events to allow note-off generation)
        handleEvent(header, const_cast<clap_output_events*>(process->out_events));
        
        // Handle note events - only process CLAP core events
        if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
            // Pass non-core events through unchanged
            const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, header);
            continue;
        }
        
        // Cast away const to allow in-place modification of note events
        clap_event_header* mutableHeader = const_cast<clap_event_header*>(header);
        
        switch (header->type) {
            case CLAP_EVENT_NOTE_ON: {
                auto ev = reinterpret_cast<clap_event_note*>(mutableHeader);
                processNoteOn(ev, const_cast<clap_output_events*>(process->out_events));
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, mutableHeader);
                break;
            }
            case CLAP_EVENT_NOTE_OFF: {
                auto ev = reinterpret_cast<clap_event_note*>(mutableHeader);
                processNoteOff(ev, const_cast<clap_output_events*>(process->out_events));
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, mutableHeader);
                break;
            }
            case CLAP_EVENT_MIDI: {
                auto ev = reinterpret_cast<clap_event_midi*>(mutableHeader);
                processMidiEvent(ev, const_cast<clap_output_events*>(process->out_events));
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, mutableHeader);
                break;
            }
            case CLAP_EVENT_MIDI_SYSEX:
            case CLAP_EVENT_MIDI2:
                // Pass MIDI system exclusive and MIDI2 events through unchanged
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, header);
                break;
            case CLAP_EVENT_NOTE_CHOKE:
            case CLAP_EVENT_NOTE_END:
            case CLAP_EVENT_NOTE_EXPRESSION:
                // Pass through unchanged
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, header);
                break;
            default:
                // Pass all other events through unchanged
                const_cast<clap_output_events*>(process->out_events)->try_push(process->out_events, header);
                break;
        }
    }
    
    return CLAP_PROCESS_CONTINUE;
}

} // namespace scale_scaler
