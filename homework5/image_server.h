#pragma once

#include <inttypes.h>
#include <stddef.h>

// starts the image server in the background for bitmap images.
// port "8000" would serve images at http://localhost:8000/image.bmp
void image_server_start(char *port);

// sets the full bmp image file data to serve
// this may be called either before or after the image server has started
// and multiple calls can be used to make an animation over time,
// so long as the client continues to refresh the image
void image_server_set_data(size_t size, uint8_t *data);
