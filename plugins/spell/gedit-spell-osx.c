#include "gedit-spell-osx.h"
#include <Cocoa/Cocoa.h>

gchar *
gedit_spell_osx_get_preferred_spell_language ()
{
	gchar *ret = NULL;
	NSAutoreleasePool *pool;
	
	pool = [[NSAutoreleasePool alloc] init];

#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5
	NSArray *langs;

	langs = [[NSSpellChecker sharedSpellChecker] userPreferredLanguages];

	if ([langs count] > 0)
	{
		ret = g_strdup ([[langs objectAtIndex:0] UTF8String]);
	}
#endif

	[pool release];
	return ret;
}

/* ex:set ts=8 noet: */
