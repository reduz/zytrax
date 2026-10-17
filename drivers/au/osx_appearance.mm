#include "osx_appearance.h"

#import <Foundation/Foundation.h>

bool osx_is_dark_mode() {
	// The system writes "Dark" into the AppleInterfaceStyle global default when
	// dark mode is active, and leaves the key absent for light mode. This works
	// without an initialized NSApplication, which we don't have under GTK.
	@autoreleasepool {
		NSString *style = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
		return style != nil && [style rangeOfString:@"Dark" options:NSCaseInsensitiveSearch].location != NSNotFound;
	}
}
