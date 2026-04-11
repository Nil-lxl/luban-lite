#pragma once

/** HTTP GET request 
 * @param url The URL to request
 * @return Pointer to response data or NULL on failure
 */
uint8_t *http_get_request(uint8_t *url);