#include "Notification.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
	void Notify(char *title, char *desc, char *appIconLocation, int timeout) {}
#elif __APPLE__
	void Notify(char *title, char *desc, char *appIconLocation, int timeout) {}
#elif __linux__
	void Notify(char *title, char *desc, char *appIconLocation, int timeout) {
		char notification[512];
		snprintf(notification, 512, "notify-send \"%s\" \"%s\" -n \"%s\" -t %d", title, desc, appIconLocation, timeout * 1000);
		system(notification);
	}
#endif