/**

MIT License

Copyright (c) 2022 Mitchell Davis <mdavisprog@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "../Windowing.h"

#import <Cocoa/Cocoa.h>

namespace Frontend
{
namespace Windowing
{

void SetMovable(void* Handle, bool Movable)
{
	NSWindow* Window = (NSWindow*)Handle;
	Window.movable = Movable ? YES : NO;
}

void SetEnabled(void* Handle, bool Enabled)
{
}

void Focus(void* Handle)
{
}

std::string OpenFileDialog(void* Handle)
{
	NSWindow* KeyWindow = [NSApp keyWindow];
	NSOpenPanel* Dialog = [NSOpenPanel openPanel];
	[Dialog setLevel:CGShieldingWindowLevel()];
	[Dialog setTitle:@"Open File"];
	[Dialog setMessage:@"Open a file"];
	[Dialog setCanChooseFiles:true];
	[Dialog setCanChooseDirectories:false];
	[Dialog setAllowsMultipleSelection:FALSE];
	[Dialog setCanCreateDirectories:TRUE];

	std::string Result;
	if ([Dialog runModal] == NSModalResponseOK)
	{
		NSURL* URL = [Dialog URL];
		Result = [[[URL path] precomposedStringWithCanonicalMapping] UTF8String];
	}

	[KeyWindow makeKeyWindow];

	return Result;
}

}
}
