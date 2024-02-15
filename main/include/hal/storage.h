#pragma once

enum StorageType {
    StorageType_None,
    StorageType_SPI,
    StorageType_SDIO,
    StorageType_MMC,
    StorageType_SD,
    StorageType_SDHC,
};

struct StorageInfo {
    char *name;
    StorageType type;
    double speed;
    uint64_t total_bytes;
    uint64_t free_bytes;

};