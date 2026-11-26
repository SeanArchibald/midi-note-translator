#include <clap/clap.h>

#include "entry_impl.h"

// CLAP entry point - this is the symbol that hosts look for. The same entry
// translation unit is compiled into each clap-first target.
extern "C" {
const CLAP_EXPORT clap_plugin_entry_t clap_entry = {
    CLAP_VERSION_INIT,
    mnt_entry_init,
    mnt_entry_deinit,
    mnt_entry_get_factory,
};
}
