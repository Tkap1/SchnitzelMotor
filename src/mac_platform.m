#include <Cocoa/Cocoa.h>

static NSWindow* window;

extern bool running;  // <- declare, but do not define

bool platform_create_window_objc(int width, int height, char* title)
{
    // Start the shared application
    [NSApplication sharedApplication];

    // Set the activation policy to regular before creating the window
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Create the main window
    CGFloat screenWidth = [[NSScreen mainScreen] frame].size.width;
    CGFloat screenHeight = [[NSScreen mainScreen] frame].size.height;
    CGFloat windowX = (screenWidth - width) / 2;
    CGFloat windowY = (screenHeight - height) / 2;
    NSRect frame = NSMakeRect(windowX, windowY, width, height);

    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    window = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [window setTitle:[NSString stringWithUTF8String:title]];
    [window setLevel:NSStatusWindowLevel]; // Set window level top-most (above all other windows)
    [window makeKeyAndOrderFront:nil];
    [window orderFrontRegardless]; // Bring the main window into focus

    // Activate the window to give it focus
    [NSApp activateIgnoringOtherApps:YES];

    return true;
}

void platform_update_window_objc()
{
    NSEvent* event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate date] inMode:NSDefaultRunLoopMode dequeue:YES]) && running) {
        // Handle other events like window resizing, closing, etc.
        [NSApp sendEvent:event];

        // Check for keyboard events
        if ([event type] == NSEventTypeKeyDown) {
            NSString *characters = [event charactersIgnoringModifiers];
            NSEventModifierFlags flags = [event modifierFlags];

            // Handle window close shortcut 'cmd + q'
            if (flags & NSEventModifierFlagCommand && [characters isEqualToString:@"q"]) {
                running = false; // Exit the application
                break; // Stop processing events
            }
        }
    }

    [NSApp updateWindows];
}

// @Note(tkap, 13/07/2023): I don't know how mac works. This may not compile
bool play_sound(Sound sound)
{
    return false;
}

bool platform_init_sound()
{
    return false;
}