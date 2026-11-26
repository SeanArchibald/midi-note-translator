#pragma once

// Entry points implemented by the static plugin library. The small entry
// translation unit (entry.cpp) re-exports these for each plugin format.
extern "C" {
bool mnt_entry_init(const char* plugin_path);
void mnt_entry_deinit(void);
const void* mnt_entry_get_factory(const char* factory_id);
}
