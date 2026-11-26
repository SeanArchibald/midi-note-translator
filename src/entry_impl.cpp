#include "entry_impl.h"

#include "plugin.h"

#include <cstring>

namespace midi_note_translator {
namespace {

// Get the count of plugin descriptors
uint32_t getPluginCount(const clap_plugin_factory* factory) {
    (void)factory;
    return 1;
}

// Get the descriptor at the given index
const clap_plugin_descriptor* getPluginDescriptor(const clap_plugin_factory* factory,
                                                  uint32_t index) {
    (void)factory;
    if (index == 0)
        return Plugin::descriptor();
    return nullptr;
}

// Create a new plugin instance
const clap_plugin* createPlugin(const clap_plugin_factory* factory,
                                const clap_host* host,
                                const char* plugin_id) {
    (void)factory;
    if (!host || !plugin_id)
        return nullptr;

    if (std::strcmp(plugin_id, Plugin::descriptor()->id) != 0)
        return nullptr;

    // Check CLAP version compatibility
    if (!clap_version_is_compatible(host->clap_version))
        return nullptr;

    auto plugin = new Plugin(host);
    return plugin->clapPlugin();
}

// Plugin factory vtable
const clap_plugin_factory s_pluginFactory = {
    getPluginCount,
    getPluginDescriptor,
    createPlugin,
};

} // namespace
} // namespace midi_note_translator

extern "C" bool mnt_entry_init(const char* plugin_path) {
    (void)plugin_path;
    return true;
}

extern "C" void mnt_entry_deinit(void) {
}

extern "C" const void* mnt_entry_get_factory(const char* factory_id) {
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &midi_note_translator::s_pluginFactory;
    return nullptr;
}
