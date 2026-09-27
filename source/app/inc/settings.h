#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

/* Основной API модуля */
bool settings_load(void);
bool settings_save(void);

#endif /* SETTINGS_H */