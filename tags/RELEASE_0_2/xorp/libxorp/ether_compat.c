#include <stdio.h>

#include "config.h"
#include "ether_compat.h"

#ifdef NEED_ETHER_ATON
struct ether_addr* 
ether_aton(const char* a) {
	uint32_t w[6];
	static uint8_t o[6];
	int i;

	if (6 != sscanf(a, "%x:%x:%x:%x:%x:%x",
			&w[0], &w[1], &w[2], &w[3], &w[4], &w[5])) {
		return NULL;
	}

	for (i = 0; i < 6; i++) {
		if (w[i] > 255) return NULL;
		o[i] = (uint8_t)w[i];
	}

	return (struct ether_addr*)o;
}

#endif /* NEED_ETHER_ATON */

#ifdef NEED_ETHER_NTOA
const char* 
ether_ntoa(const struct ether_addr* ea) {
	const uint8_t *o = (const uint8_t*)ea;
	static char s[18];
	int wrote;

	wrote = sprintf(s, "%x:%x:%x:%x:%x:%x",
			o[0], o[1], o[2], o[3], o[4], o[5]);
	if (wrote < 11) {
		return NULL;
	}
	return s;
}
#endif
