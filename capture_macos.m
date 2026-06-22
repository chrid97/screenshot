#import "capture.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

struct Screenshot capture_screen(void) {
    __block struct Screenshot result = { 0 };
    @autoreleasepool {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);

        [SCShareableContent
            getShareableContentExcludingDesktopWindows:NO
                                   onScreenWindowsOnly:YES
                                     completionHandler:^(SCShareableContent *content,
                                                         NSError *error) {
                                       if (error) {
                                           NSLog(@"error: %@", error);
                                           dispatch_semaphore_signal(sem);
                                           return;
                                       }

                                       // Get first display
                                       SCDisplay *display = content.displays.firstObject;
                                       if (display == nil) {
                                           NSLog(@"no displays found");
                                           dispatch_semaphore_signal(sem);
                                           return;
                                       }

                                       SCContentFilter *filter =
                                           [[SCContentFilter alloc] initWithDisplay:display
                                                                   excludingWindows:@[]];

                                       SCStreamConfiguration *config =
                                           [[SCStreamConfiguration alloc] init];

                                       // Scale up for my Retina resoluton
                                       NSScreen *screen = NSScreen.mainScreen;
                                       result.scale = screen.backingScaleFactor;

                                       config.width = display.width * result.scale;
                                       config.height = display.height * result.scale;
                                       config.showsCursor = NO;

                                       [SCScreenshotManager
                                           captureImageWithFilter:filter
                                                    configuration:config
                                                completionHandler:^(CGImageRef image,
                                                                    NSError *error) {
                                                  if (error) {
                                                      NSLog(@"screenshot error: %@", error);
                                                      dispatch_semaphore_signal(sem);
                                                      return;
                                                  }

                                                  if (image == NULL) {
                                                      NSLog(@"screenshot returned NULL image");
                                                      dispatch_semaphore_signal(sem);
                                                      return;
                                                  }

                                                  size_t width = CGImageGetWidth(image);
                                                  size_t height = CGImageGetHeight(image);
                                                  size_t bytes_per_pixel = 4;
                                                  size_t bytes_per_row = width * bytes_per_pixel;
                                                  size_t size = bytes_per_row * height;

                                                  unsigned char *pixels = malloc(size);
                                                  CGColorSpaceRef color_space =
                                                      CGColorSpaceCreateDeviceRGB();

                                                  CGContextRef context = CGBitmapContextCreate(
                                                      pixels,
                                                      width,
                                                      height,
                                                      8,
                                                      bytes_per_row,
                                                      color_space,
                                                      (CGBitmapInfo)kCGImageAlphaPremultipliedLast);

                                                  CGColorSpaceRelease(color_space);

                                                  if (context == NULL) {
                                                      NSLog(@"failed to create bitmap context");
                                                      free(pixels);
                                                      dispatch_semaphore_signal(sem);
                                                      return;
                                                  }

                                                  CGRect rect = CGRectMake(0, 0, width, height);
                                                  CGContextDrawImage(context, rect, image);
                                                  CGContextRelease(context);

                                                  NSLog(
                                                      @"copied image into pixel buffer: %zu bytes",
                                                      size);

                                                  result.width = (int)width;
                                                  result.height = (int)height;
                                                  result.stride = (int)bytes_per_row;
                                                  result.pixels = pixels;

                                                  dispatch_semaphore_signal(sem);
                                                }];
                                     }];

        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }
    return result;
}

void copy_png_to_clipboard(const unsigned char *data, int size) {
    NSData *png = [NSData dataWithBytes:data length:size];

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

    [pasteboard clearContents];

    [pasteboard setData:png forType:NSPasteboardTypePNG];
}
