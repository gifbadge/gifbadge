#pragma once
void storage_init();
bool storage_free();

typedef void (*mount_callback)(bool);
void storage_callback(mount_callback);