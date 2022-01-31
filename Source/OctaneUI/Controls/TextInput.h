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

#pragma once

#include "Control.h"

namespace OctaneUI
{

class Text;

class TextInput : public Control
{
	CLASS(TextInput)

public:
	TextInput(Window* InWindow);
	virtual ~TextInput();

	TextInput* SetText(const char* InText);
	const char* GetText() const;

	virtual void OnPaint(Paint& Brush) const override;
	virtual void Update() override;
	virtual void OnFocused() override;
	virtual void OnUnfocused() override;
	virtual void OnKeyPressed(Keyboard::Key Key) override;
	virtual bool OnMousePressed(const Vector2& Position, Mouse::Button Button) override;
	virtual void OnText(uint32_t Code) override;

private:
	void Backspace();
	void MovePosition(int32_t Count);

	std::shared_ptr<Text> m_Text;
	uint32_t m_Position;
	bool m_Focused;
};

}
