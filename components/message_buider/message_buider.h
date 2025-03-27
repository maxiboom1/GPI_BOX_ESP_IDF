#pragma once

#include <stddef.h>

char* construct_message(const char *event, const char *state, const char *user, const char *password, char *out_buffer, size_t buffer_size);
