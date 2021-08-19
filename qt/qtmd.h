#if defined( __sparc ) || defined( __sparc__ )
#include "md/sparc.h"
#elif defined( __hppa )
#include "md/hppa.h"
#elif defined( __x86_64__ )
#include "md/iX86_64.h"
#elif defined( __i386 )
#include "md/i386.h"
#elif defined( __ppc__ )
#include "md/powerpc_mach.h"
#elif defined( __powerpc )
#include "md/powerpc_sys5.h"
#elif defined( __aarch64__ )
#include "md/aarch64.h"
#endif
