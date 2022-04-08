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

#include "TextInput.h"
#include "../Font.h"
#include "../Json.h"
#include "../Paint.h"
#include "../Theme.h"
#include "../Timer.h"
#include "../Window.h"
#include "MarginContainer.h"
#include "ScrollableContainer.h"
#include "Text.h"

namespace OctaneGUI
{

class TextInputInteraction : public Control
{
	CLASS(TextInputInteraction)

public:
	TextInputInteraction(Window* InWindow, TextInput* Input)
		: Control(InWindow)
		, m_Input(Input)
	{
		SetExpand(Expand::Both);
		SetForwardKeyEvents(true);
	}

	virtual void OnFocused() override
	{
		m_Input->Focus();
	}

	virtual void OnUnfocused() override
	{
		m_Input->Unfocus();
	}

	virtual bool OnKeyPressed(Keyboard::Key Key) override
	{
		if (Control::OnKeyPressed(Key))
		{
			return true;
		}

		m_Input->m_DrawCursor = true;
		m_Input->m_BlinkTimer->Stop();

		switch (Key)
		{
		case Keyboard::Key::Backspace: m_Input->Delete(m_Input->GetRangeOr(-1)); return true;
		case Keyboard::Key::Delete: m_Input->Delete(m_Input->GetRangeOr(1)); return true;
		case Keyboard::Key::Left: m_Input->MovePosition(0, -1, m_Input->IsShiftPressed()); return true;
		case Keyboard::Key::Right: m_Input->MovePosition(0, 1, m_Input->IsShiftPressed()); return true;
		case Keyboard::Key::Up: m_Input->MovePosition(-1, 0, m_Input->IsShiftPressed()); return true;
		case Keyboard::Key::Down: m_Input->MovePosition(1, 0, m_Input->IsShiftPressed()); return true;
		case Keyboard::Key::Home: m_Input->MoveHome(); return true;
		case Keyboard::Key::End: m_Input->MoveEnd(); return true;
		case Keyboard::Key::Enter: m_Input->AddText('\n'); return true;
		default: break;
		}

		return false;
	}

	virtual void OnKeyReleased(Keyboard::Key Key) override
	{
		Control::OnKeyReleased(Key);
		m_Input->ResetCursorTimer();
	}

	virtual void OnMouseMove(const Vector2& Position) override
	{
		m_Input->MouseMove(Position);
	}

	virtual bool OnMousePressed(const Vector2& Position, Mouse::Button Button) override
	{
		return m_Input->MousePressed(Position, Button);
	}

	virtual void OnMouseReleased(const Vector2& Position, Mouse::Button Button) override
	{
		m_Input->MouseReleased(Position, Button);
	}

	virtual void OnText(uint32_t Code) override
	{
		m_Input->AddText(Code);
	}

private:
	TextInput* m_Input { nullptr };
};

TextInput::TextPosition::TextPosition()
{
}

TextInput::TextPosition::TextPosition(uint32_t Line, uint32_t Column, uint32_t Index)
	: m_Line(Line)
	, m_Column(Column)
	, m_Index(Index)
{
}

bool TextInput::TextPosition::operator==(const TextInput::TextPosition& Other) const
{
	return m_Line == Other.m_Line && m_Column == Other.m_Column && m_Index == Other.m_Index;
}

bool TextInput::TextPosition::operator!=(const TextInput::TextPosition& Other) const
{
	return m_Line != Other.m_Line || m_Column != Other.m_Column || m_Index != Other.m_Index;
}

bool TextInput::TextPosition::operator<(const TextInput::TextPosition& Other) const
{
	return m_Line < Other.m_Line || (m_Line == Other.m_Line && m_Column < Other.m_Column);
}

void TextInput::TextPosition::Invalidate()
{
	m_Line = -1;
	m_Column = -1;
	m_Index = -1;
}

bool TextInput::TextPosition::IsValid() const
{
	return m_Line != -1 && m_Column != -1 && m_Index != -1;
}

bool TextInput::TextPosition::IsValidIndex() const
{
	return m_Index != -1;
}

void TextInput::TextPosition::SetLine(uint32_t Line)
{
	m_Line = Line;
}

uint32_t TextInput::TextPosition::Line() const
{
	return m_Line;
}

void TextInput::TextPosition::SetColumn(uint32_t Column)
{
	m_Column = Column;
}

uint32_t TextInput::TextPosition::Column() const
{
	return m_Column;
}

void TextInput::TextPosition::SetIndex(uint32_t Index)
{
	m_Index = Index;
}

uint32_t TextInput::TextPosition::Index() const
{
	return m_Index;
}

TextInput::TextInput(Window* InWindow)
	: ScrollableViewControl(InWindow)
{
	std::shared_ptr<MarginContainer> Margins = Scrollable()->AddControl<MarginContainer>();
	Margins->SetMargins({ 2.0, 0.0, 2.0f, 0.0f });
	m_Text = Margins->AddControl<Text>();

	AddControl<TextInputInteraction>(this);

	SetSize({ 100.0f, m_Text->LineHeight() });

	m_BlinkTimer = InWindow->CreateTimer(500, true, [this]() -> void
		{
			m_DrawCursor = !m_DrawCursor;
			Invalidate();
		});
}

TextInput::~TextInput()
{
}

TextInput& TextInput::SetText(const char* InText)
{
	InternalSetText(InText);
	m_Anchor.Invalidate();
	m_Position = { 0, 0, 0 };
	UpdateFormats();
	Invalidate();
	return *this;
}

const char* TextInput::GetText() const
{
	return m_Text->GetText();
}

TextInput& TextInput::SetReadOnly(bool Value)
{
	if (m_ReadOnly == Value)
	{
		return *this;
	}

	m_ReadOnly = Value;
	Invalidate();
	return *this;
}

bool TextInput::ReadOnly() const
{
	return m_ReadOnly;
}

void TextInput::Focus()
{
	if (!m_Position.IsValid())
	{
		m_Position = { 0, 0, 0 };
	}

	m_Focused = true;
	ResetCursorTimer();
	Invalidate();
}

void TextInput::Unfocus()
{
	m_Focused = false;
	m_BlinkTimer->Stop();
	Invalidate();
}

void TextInput::OnPaint(Paint& Brush) const
{
	std::shared_ptr<Theme> TheTheme = GetTheme();
	const float LineHeight = m_Text->LineHeight();

	Rect Bounds = GetAbsoluteBounds();

	if (GetProperty(ThemeProperties::TextInput_3D).Bool())
	{
		Brush.Rectangle3D(
			Bounds,
			GetProperty(ThemeProperties::TextInput_Background).ToColor(),
			GetProperty(ThemeProperties::Button_Highlight_3D).ToColor(),
			GetProperty(ThemeProperties::Button_Shadow_3D).ToColor(),
			true);
	}
	else
	{
		Brush.Rectangle(Bounds, GetProperty(ThemeProperties::TextInput_Background).ToColor());
	}

	if (m_Focused)
	{
		Brush.RectangleOutline(Bounds, GetProperty(ThemeProperties::TextInput_FocusedOutline).ToColor());
	}

	Brush.PushClip(GetAbsoluteBounds());

	if (m_Anchor.IsValid() && m_Anchor != m_Position)
	{
		const TextPosition Min = m_Anchor < m_Position ? m_Anchor : m_Position;
		const TextPosition Max = m_Anchor < m_Position ? m_Position : m_Anchor;

		const std::string& String = m_Text->GetString();
		if (Min.Line() == Max.Line())
		{
			const Vector2 MinPos = GetPositionLocation(Min);
			const Vector2 MaxPos = GetPositionLocation(Max);

			const Rect SelectBounds = {
				m_Text->GetAbsolutePosition() + MinPos,
				m_Text->GetAbsolutePosition() + MaxPos + Vector2(0.0f, LineHeight)
			};

			Brush.Rectangle(SelectBounds, GetProperty(ThemeProperties::TextInput_Selection).ToColor());
		}
		else
		{
			uint32_t Index = Min.Index();
			for (uint32_t Line = Min.Line(); Line <= Max.Line(); Line++)
			{
				if (Line == Min.Line())
				{
					const std::string Sub = String.substr(Min.Index(), LineEndIndex(Min.Index()) - Min.Index());
					const Vector2 Size = m_Text->GetFont()->Measure(Sub);
					const Vector2 Position = GetPositionLocation(Min);
					const Rect SelectBounds = {
						m_Text->GetAbsolutePosition() + Position,
						m_Text->GetAbsolutePosition() + Position + Vector2(Size.X, LineHeight)
					};
					Brush.Rectangle(SelectBounds, GetProperty(ThemeProperties::TextInput_Selection).ToColor());
				}
				else if (Line == Max.Line())
				{
					const uint32_t Start = LineStartIndex(Max.Index());
					const std::string Sub = String.substr(Start, Max.Index() - Start);
					const Vector2 Size = m_Text->GetFont()->Measure(Sub);
					const Vector2 Position = GetPositionLocation(Max);
					const Rect SelectBounds = {
						m_Text->GetAbsolutePosition() + Position - Vector2(Size.X, 0.0f),
						m_Text->GetAbsolutePosition() + Position + Vector2(0.0f, LineHeight)
					};
					Brush.Rectangle(SelectBounds, GetProperty(ThemeProperties::TextInput_Selection).ToColor());
				}
				else
				{
					const std::string Sub = String.substr(Index, LineEndIndex(Index) - Index);
					const Vector2 Size = m_Text->GetFont()->Measure(Sub);
					const Vector2 Position = GetPositionLocation({ Line, 0, Index });
					const Rect SelectBounds = {
						m_Text->GetAbsolutePosition() + Position,
						m_Text->GetAbsolutePosition() + Position + Vector2(Size.X, LineHeight)
					};
					Brush.Rectangle(SelectBounds, GetProperty(ThemeProperties::TextInput_Selection).ToColor());
				}

				Index = LineEndIndex(Index) + 1;
			}
		}
	}

	if (m_Focused && m_DrawCursor)
	{
		const Vector2 Size = GetPositionLocation(m_Position);
		const Vector2 Start = m_Text->GetAbsolutePosition() + Vector2(std::max<float>(Size.X, 2.0f), Size.Y);
		const Vector2 End = Start + Vector2(0.0f, LineHeight);
		Brush.Line(Start, End, GetProperty(ThemeProperties::TextInput_Cursor).ToColor());
	}

	Brush.PopClip();

	Container::OnPaint(Brush);
}

void TextInput::OnLoad(const Json& Root)
{
	Container::OnLoad(Root);

	m_Multiline = Root["Multiline"].Boolean();
	if (m_Multiline)
	{
		SetSize({ 200.0f, 200.0f });
	}
	Scrollable()->SetHorizontalSBEnabled(m_Multiline).SetVerticalSBEnabled(m_Multiline);

	m_Text->OnLoad(Root["Text"]);
}

void TextInput::MouseMove(const Vector2& Position)
{
	if (m_Drag)
	{
		m_Position = GetPosition(Position);
		Invalidate();
		UpdateFormats();
	}
	else
	{
		Scrollable()->OnMouseMove(Position);
	}
}

bool TextInput::MousePressed(const Vector2& Position, Mouse::Button Button)
{
	if (Scrollable()->OnMousePressed(Position, Button))
	{
		return true;
	}

	m_Position = GetPosition(Position);
	m_Anchor = m_Position;
	m_Drag = true;
	m_Text->ClearFormats();
	ResetCursorTimer();
	Invalidate();

	return true;
}

void TextInput::MouseReleased(const Vector2& Position, Mouse::Button Button)
{
	if (m_Anchor == m_Position)
	{
		m_Anchor.Invalidate();
	}

	m_Drag = false;
	Scrollable()->OnMouseReleased(Position, Button);
}

void TextInput::AddText(uint32_t Code)
{
	if (m_ReadOnly)
	{
		return;
	}

	if (!std::isalnum(Code) && Code != '\n' && Code != ' ')
	{
		return;
	}

	// Prevent newline in single-line inputs.
	if (!m_Multiline && Code == '\n')
	{
		return;
	}

	if (m_Anchor.IsValid())
	{
		Delete(GetRangeOr(0));
	}

	std::string Contents = m_Text->GetText();
	Contents.insert(Contents.begin() + m_Position.Index(), (int8_t)Code);
	InternalSetText(Contents.c_str());
	Scrollable()->Update();
	MovePosition(0, 1);
	ResetCursorTimer();
}

void TextInput::Delete(int32_t Range)
{
	uint32_t Index = m_Position.Index();

	int32_t Min = std::min<int32_t>(Index, Index + (int32_t)Range);
	Min = std::max<int32_t>(0, Min);

	int32_t Max = std::max<int32_t>(Index, Index + (int32_t)Range);
	Max = std::min<int32_t>(m_Text->Length(), Max);

	// Only move the cursor if deleting characters to the left of the cursor.
	// Move the position before updating the text object. This should place
	// the position to the correct index in the string buffer.
	int32_t Move = std::min<int32_t>(Range, 0);
	MovePosition(0, Move);

	// TODO: Maybe allow altering the contents in-place and repaint?
	std::string Contents = m_Text->GetText();
	Contents.erase(Contents.begin() + (uint32_t)Min, Contents.begin() + (uint32_t)Max);

	InternalSetText(Contents.c_str());
	Scrollable()->Update();
	ResetCursorTimer();
}

void TextInput::MoveHome()
{
	int Index = LineStartIndex(m_Position.Index());
	Index = Index > 0 ? Index + 1 : Index;
	MovePosition(0, Index - m_Position.Index(), IsShiftPressed());
}

void TextInput::MoveEnd()
{
	MovePosition(0, LineEndIndex(m_Position.Index()) - m_Position.Index(), IsShiftPressed());
}

void TextInput::MovePosition(int32_t Line, int32_t Column, bool UseAnchor)
{
	// This function will calculate the new line and column along with the index
	// into the string buffer of the text object.

	if (UseAnchor)
	{
		if (!m_Anchor.IsValid())
		{
			m_Anchor = m_Position;
		}
	}
	else
	{
		m_Anchor.Invalidate();
	}

	const std::string& String = m_Text->GetString();

	// Prevent any update if trying to go before the beginning or moveing past the end.
	if ((Line < 0 && m_Position.Line() == 0) || (Column < 0 && m_Position.Index() == 0) || (Column > 0 && m_Position.Index() == String.size()))
	{
		return;
	}

	int32_t NewIndex = m_Position.Index();
	int32_t LineIndex = LineStartIndex(m_Position.Index());
	int32_t NewLine = m_Position.Line();
	int32_t NewColumn = m_Position.Column();

	// First, figure out the line and the string index for the new line.
	// This is done by iterating each line and calculating the new offset until the desired number
	// of lines is reached.
	const bool LineBack = Line < 0;
	// No need to move the index forward if the current index is zero.
	if (m_Position.Index() > 0)
	{
		LineIndex = String[LineIndex] == '\n' ? LineIndex + 1 : LineIndex;
	}

	for (int32_t I = 0; I < std::abs(Line) && NewLine >= 0; I++)
	{
		// Need to adjust the starting search position for finding the next newline character.
		// Want to avoid cases where the same index is returned.
		const uint32_t Start = LineBack ? std::max<int32_t>(LineIndex - 1, 0) : LineIndex;
		uint32_t Index = LineBack ? LineStartIndex(Start) : LineEndIndex(Start);

		if (Index == String.size())
		{
			NewIndex = String.size();
			break;
		}
		// Catching an edge case here where the first line could end up being a single newline.
		// If this is the case, Index will actually end up being a newline because of the above
		// logic pushing the index forward if it is on a newline character. This should occur
		// for normal newlines in the middle of the buffer, but not at the beginning.
		else if (Start == 0 && LineBack)
		{
			Index = 0;
		}
		else
		{
			Index = String[Index] == '\n' ? Index + 1 : Index;
		}

		NewLine = LineBack ? NewLine - 1 : NewLine + 1;
		NewIndex = Index;
		LineIndex = Index;
	}

	NewColumn = std::min<int32_t>(NewColumn, LineSize(LineIndex));
	NewIndex = LineIndex + NewColumn;

	// Apply any column movement. This will alter the current line based on if the
	// cursor moves past the beginning or end of a line. This is done by looping
	// and subtracting from the amount of column spaces to move until all moves
	// have been accounted for.
	const bool ColumnBack = Column < 0;
	Column = std::abs(Column);
	int32_t ColumnIndex = NewIndex + Column;
	while (Column != 0)
	{
		// Find the line character based on if the cursor is moving forward or backward.
		int LineSize = this->LineSize(NewIndex);
		uint32_t Index = ColumnBack ? LineStartIndex(NewIndex) : LineEndIndex(NewIndex);

		// Prevent the diff to exceed amount of columns to traverse.
		int Diff = std::min<int>(std::abs(NewIndex - (int)Index), std::abs(Column));
		if (Diff == 0)
		{
			// Search for newline characters will not result in a Diff, so apply
			// one to move past this character. This will force the line count to
			// update.
			if (String[NewIndex] == '\n')
			{
				Diff = 1;
			}
			else
			{
				// May be trying to move at the beginning or end.
				Column = 0;
			}
		}

		Column = std::max<int>(Column - Diff, 0);

		// Apply the current diff amount to the index for possible further searches.
		// Clamp to [0, Stirng.size]
		NewIndex = ColumnBack
			? std::max<int>(NewIndex - Diff, 0)
			: std::min<int>(NewIndex + Diff, String.size());

		// Set the new column index. This will move the column index to either the beginning
		// or end of a line if the column exceeds the line size.
		NewColumn = ColumnBack ? NewColumn - Diff : NewColumn + Diff;
		if (NewColumn < 0)
		{
			NewLine--;
			NewColumn = this->LineSize(Index);
		}
		else if (NewColumn > LineSize)
		{
			NewLine++;
			NewColumn = 0;
		}
	}

	NewLine = std::max<int32_t>(NewLine, 0);
	NewColumn = std::max<int>(NewColumn, 0);

	m_Position = { (uint32_t)NewLine, (uint32_t)NewColumn, (uint32_t)NewIndex };
	ScrollIntoView();
	UpdateFormats();
	Invalidate();
}

Vector2 TextInput::GetPositionLocation(const TextPosition& Position) const
{
	if (!m_Position.IsValid())
	{
		return { 0.0f, 0.0f };
	}

	const std::string& String = m_Text->GetString();
	uint32_t Start = LineStartIndex(Position.Index());

	const std::string Sub = String.substr(Start, Position.Index() - Start);
	return { m_Text->GetFont()->Measure(Sub).X, Position.Line() * m_Text->LineHeight() };
}

TextInput::TextPosition TextInput::GetPosition(const Vector2& Position) const
{
	const float LineHeight = m_Text->LineHeight();
	const std::string& String = m_Text->GetString();

	// Transform into local space.
	const Vector2 LocalPosition = Position - Scrollable()->GetAbsolutePosition();
	// The text control is offset by a margin container so this needs to be taken into account.
	const Vector2 TextOffset = m_Text->GetAbsolutePosition() - Scrollable()->GetAbsolutePosition();

	// Find the starting index based on what line the position is on.
	size_t StartIndex = 0;
	size_t Index = 0;
	uint32_t Line = 0;
	uint32_t Column = 0;
	Vector2 Offset = { 0.0f, LineHeight };
	while (StartIndex != std::string::npos)
	{
		if (Offset.Y > LocalPosition.Y + TextOffset.Y)
		{
			Index = StartIndex;
			break;
		}

		size_t Find = String.find('\n', StartIndex);
		if (Find != std::string::npos)
		{
			Line++;
			StartIndex = Find + 1;
			Offset.Y += LineHeight;
		}
		else
		{
			// Reached the end of the string. Mark the column to be the end
			// of the final line and make the index be the size of the string.
			Column = String.size() - StartIndex;
			Index = String.size();
			break;
		}
	}

	// Find the character on the line that is after the given position.
	for (; Index < String.size(); Index++, Column++)
	{
		const char Ch = String[Index];
		if (Ch == '\n')
		{
			break;
		}

		const Vector2 Size = GetTheme()->GetFont()->Measure(Ch);
		Offset.X += Size.X;

		if (Position.X - Scrollable()->GetPosition().X - TextOffset.X <= GetAbsolutePosition().X + Offset.X)
		{
			break;
		}
	}

	return { Line, Column, (uint32_t)Index };
}

bool TextInput::IsShiftPressed() const
{
	return IsKeyPressed(Keyboard::Key::LeftShift) || IsKeyPressed(Keyboard::Key::RightShift);
}

int32_t TextInput::GetRangeOr(int32_t Value) const
{
	if (!m_Anchor.IsValid())
	{
		return Value;
	}

	return m_Anchor.Index() - m_Position.Index();
}

uint32_t TextInput::LineStartIndex(uint32_t Index) const
{
	const std::string& String = m_Text->GetString();

	// The index may already be on a newline character. Start the search at the character
	// before this one.
	const uint32_t Offset = String[Index] == '\n' ? std::max<int>(Index - 1, 0) : Index;
	size_t Result = String.rfind('\n', Offset);
	return Result == std::string::npos ? 0 : Result;
}

uint32_t TextInput::LineEndIndex(uint32_t Index) const
{
	const std::string& String = m_Text->GetString();

	if (String[Index] == '\n')
	{
		return Index;
	}

	size_t Result = String.find('\n', Index);
	return Result == std::string::npos ? String.size() : Result;
}

uint32_t TextInput::LineSize(uint32_t Index) const
{
	const std::string& String = m_Text->GetString();
	uint32_t Start = LineStartIndex(Index);
	// The line should start at the character after the newline character.
	if (String[Start] == '\n')
	{
		Start++;
	}
	uint32_t End = LineEndIndex(Index);
	return std::max<int>((int)End - (int)Start, 0);
}

void TextInput::ScrollIntoView()
{
	const float LineHeight = m_Text->LineHeight();
	const Vector2 TextOffset = m_Text->GetAbsolutePosition() - Scrollable()->GetAbsolutePosition();
	const Vector2 Position = GetPositionLocation(m_Position) + Scrollable()->GetPosition() + TextOffset;
	const Vector2 Size = Scrollable()->GetScrollableSize();
	Vector2 Offset;

	if (Position.X < 0.0f)
	{
		Offset.X = Position.X - 2.0f - TextOffset.X;
	}
	if (Position.Y < 0.0f)
	{
		Offset.Y = Position.Y - TextOffset.Y;
	}

	if (Position.X > Size.X)
	{
		Offset.X = Position.X - Size.X + TextOffset.X;
	}
	if (Position.Y + LineHeight > Size.Y)
	{
		Offset.Y = Position.Y + LineHeight - Size.Y + TextOffset.Y;
	}

	Scrollable()->AddOffset(Offset);
}

void TextInput::UpdateFormats()
{
	m_Text->ClearFormats();

	if (!m_Anchor.IsValid())
	{
		return;
	}

	if (m_Anchor != m_Position)
	{
		TextPosition Min = m_Anchor < m_Position ? m_Anchor : m_Position;
		TextPosition Max = m_Anchor < m_Position ? m_Position : m_Anchor;
		m_Text->PushFormat({ 0, Min.Index(), GetProperty(ThemeProperties::Text).ToColor() });
		m_Text->PushFormat({ Min.Index(), Max.Index(), GetProperty(ThemeProperties::TextSelectable_Text_Hovered).ToColor() });
		m_Text->PushFormat({ Max.Index(), m_Text->Length(), GetProperty(ThemeProperties::Text).ToColor() });
	}
}

void TextInput::InternalSetText(const char* InText)
{
	m_Text->SetText(InText);
	Invalidate();
}

void TextInput::ResetCursorTimer()
{
	m_BlinkTimer->Start();
	m_DrawCursor = true;
}

}