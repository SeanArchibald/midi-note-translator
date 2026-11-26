#include "plugin.h"
#include <clap/clap.h>
#include <cstring>

namespace midi_note_translator {

// Get the count of plugin descriptors
static uint32_t getPluginCount(const clap_plugin_factory* factory) {
    (void)factory;
    return 1;
}

// Get the descriptor at the given index
static const clap_plugin_descriptor* getPluginDescriptor(const clap_plugin_factory* factory,
                                                          uint32_t index) {
    (void)factory;
    if (index == 0)
        return Plugin::descriptor();
    return nullptr;
}

// Create a new plugin instance
static const clap_plugin* createPlugin(const clap_plugin_factory* factory,
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
static const clap_plugin_factory s_pluginFactory = {
    getPluginCount,
    getPluginDescriptor,
    createPlugin,
};

// Entry init function
static bool entryInit(const char* plugin_path) {
    (void)plugin_path;
    return true;
}

// Entry deinit function
static void entryDeinit(void) {
}

// Entry get_factory function
static const void* entryGetFactory(const char* factory_id) {
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &s_pluginFactory;
    return nullptr;
}

} // namespace midi_note_translator

// CLAP entry point - this is the symbol that hosts look for
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION_INIT,
    midi_note_translator::entryInit,
    midi_note_translator::entryDeinit,
    midi_note_translator::entryGetFactory,
};
