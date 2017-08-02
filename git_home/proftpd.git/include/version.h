#include "buildstamp.h"

/* Application version (in various forms) */
#define PROFTPD_VERSION_NUMBER		0x0001030508
#define PROFTPD_VERSION_TEXT		"1.3.5V"

/* Module API version */
#define PR_MODULE_API_VERSION		0x20

unsigned long pr_version_get_module_api_number(void);
unsigned long pr_version_get_number(void);
const char *pr_version_get_str(void);

/* PR_STATUS is reported by --version-status -- don't ask why */
#define PR_STATUS          		"(maint)"
