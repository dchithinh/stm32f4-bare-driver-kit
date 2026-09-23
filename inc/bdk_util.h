#ifndef BDK_UTIL_H
#define BDK_UTIL_H

/**
 * @brief Number of elements in an array (not a pointer).
 *
 * Pass the array object, e.g. `BDK_ARRAY_LEN(usart_clk)`.
 * Do not pass a pointer: `sizeof` would be the pointer width.
 */
#define BDK_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#endif /* BDK_UTIL_H */
