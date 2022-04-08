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

#include "Paint.h"
#include "Application.h"
#include "Font.h"
#include "Rect.h"
#include "Texture.h"
#include "Theme.h"

#include <string_view>

namespace OctaneGUI
{

Paint::Paint()
	: m_Buffer()
	, m_Theme(nullptr)
{
}

Paint::Paint(const std::shared_ptr<Theme>& InTheme)
	: m_Theme(InTheme)
{
}

Paint::~Paint()
{
}

void Paint::Line(const Vector2& Start, const Vector2& End, const Color& Col, float Thickness)
{
	PushCommand(6, 0);
	AddLine(Start, End, Col, Thickness);
}

void Paint::Rectangle(const Rect& Bounds, const Color& Col)
{
	PushCommand(6, 0);
	AddTriangles(Bounds, Col);
}

void Paint::Rectangle3D(const Rect& Bounds, const Color& Base, const Color& Highlight, const Color& Shadow, bool Sunken)
{
	Rectangle(Bounds, Base);

	const float Thickness = Sunken ? 1.0f : 1.0f;
	const float HThickness = Thickness * 0.5f;

	const Vector2 Min = Bounds.Min + Vector2(HThickness, HThickness);
	Vector2 TR = Min + Vector2(Bounds.Width() - HThickness, 0.0f);
	Vector2 BL = Min + Vector2(0.0f, Bounds.Height() - HThickness);
	Line(Min, TR, Sunken ? Shadow : Highlight, Thickness);
	Line(Min, BL, Sunken ? Shadow : Highlight, Thickness);

	const Vector2 Max = Bounds.Max - Vector2(HThickness, HThickness);
	TR = Max - Vector2(0.0f, Bounds.Height() - HThickness);
	BL = Max - Vector2(Bounds.Width() - HThickness, 0.0f);
	Line(Max, TR, Sunken ? Highlight : Shadow, Thickness);
	Line(Max, BL, Sunken ? Highlight : Shadow, Thickness);
}

void Paint::RectangleOutline(const Rect& Bounds, const Color& Col, float Thickness)
{
	const Vector2 TopRight(Bounds.Min + Vector2(Bounds.GetSize().X, 0.0f));
	const Vector2 BottomLeft(Bounds.Min + Vector2(0.0f, Bounds.GetSize().Y));

	PushCommand(6 * 4, 0);
	AddLine(Bounds.Min, TopRight, Col, Thickness, 0);
	AddLine(TopRight, Bounds.Max, Col, Thickness, 4);
	AddLine(Bounds.Max, BottomLeft, Col, Thickness, 8);
	AddLine(BottomLeft, Bounds.Min, Col, Thickness, 12);
}

size_t StripInvalidCharactersLength(const std::string_view& Contents)
{
	size_t Start = 0;
	size_t InvalidChars = 0;
	while (Start < Contents.size())
	{
		size_t End = Contents.find_first_of("\n", Start);
		if (End != std::string::npos)
		{
			InvalidChars++;
			Start = End + 1;
		}
		else
		{
			Start = Contents.size();
		}
	}

	return InvalidChars;
}

void Paint::Text(const std::shared_ptr<Font>& InFont, const Vector2& Position, const std::string& Contents, const Color& Col)
{
	if (Contents.empty())
	{
		return;
	}

	size_t InvalidChars = StripInvalidCharactersLength(Contents);
	size_t Count = Contents.size() - InvalidChars;
	PushCommand(6 * Count, InFont->ID());
	Vector2 Pos = Position;
	uint32_t Offset = 0;
	for (char Char : Contents)
	{
		if (Char == '\n')
		{
			Pos.X = Position.X;
			Pos.Y += InFont->Size();
			continue;
		}

		Rect Vertices;
		Rect TexCoords;

		InFont->Draw((int32_t)Char - 32, Pos, Vertices, TexCoords);
		AddTriangles(Vertices, TexCoords, Col, Offset);
		Offset += 4;
	}
}

void Paint::Textf(const std::shared_ptr<Font>& InFont, const Vector2& Position, const std::string& Contents, const std::vector<TextFormat>& Formats)
{
	if (Contents.empty())
	{
		return;
	}

	size_t InvalidChars = 0;
	std::vector<std::string_view> Views;
	for (const TextFormat& Item : Formats)
	{
		Views.emplace_back(&Contents[Item.Start], Item.End - Item.Start);
		InvalidChars += StripInvalidCharactersLength(Views.back());
	}
	size_t Count = Contents.size() - InvalidChars;
	PushCommand(6 * Count, InFont->ID());

	Vector2 Pos = Position;
	uint32_t Offset = 0;
	for (size_t I = 0; I < Formats.size(); I++)
	{
		const TextFormat& Format = Formats[I];
		const std::string_view& View = Views[I];
		for (char Char : View)
		{
			if (Char == '\n')
			{
				Pos.X = Position.X;
				Pos.Y += InFont->Size();
				continue;
			}

			Rect Vertices;
			Rect TexCoords;

			InFont->Draw((int32_t)Char - 32, Pos, Vertices, TexCoords);
			AddTriangles(Vertices, TexCoords, Format.TextColor, Offset);
			Offset += 4;
		}
	}
}

void Paint::Image(const Rect& Bounds, const Rect& TexCoords, const std::shared_ptr<Texture>& InTexture, const Color& Col)
{
	if (!InTexture)
	{
		return;
	}

	PushCommand(6, InTexture->GetID());
	AddTriangles(Bounds, TexCoords, Col);
}

void Paint::PushClip(const Rect& Bounds)
{
	m_ClipStack.push_back(Bounds);
}

void Paint::PopClip()
{
	m_ClipStack.pop_back();
}

const VertexBuffer& Paint::GetBuffer() const
{
	return m_Buffer;
}

std::shared_ptr<Theme> Paint::GetTheme() const
{
	return m_Theme;
}

void Paint::AddLine(const Vector2& Start, const Vector2& End, const Color& Col, float Thickness, uint32_t Offset)
{
	const Vector2 Direction = (End - Start).Unit();
	const float HalfThickness = Thickness * 0.5f;

	m_Buffer.AddVertex(Start + Vector2(-Direction.Y, Direction.X) * HalfThickness, Col);
	m_Buffer.AddVertex(End + Vector2(-Direction.Y, Direction.X) * HalfThickness, Col);
	m_Buffer.AddVertex(End + Vector2(Direction.Y, -Direction.X) * HalfThickness, Col);
	m_Buffer.AddVertex(Start + Vector2(Direction.Y, -Direction.X) * HalfThickness, Col);

	AddTriangleIndices(Offset);
}

void Paint::AddTriangles(const Rect& Vertices, const Color& Col, uint32_t Offset)
{
	m_Buffer.AddVertex(Vertices.Min, Col);
	m_Buffer.AddVertex(Vector2(Vertices.Max.X, Vertices.Min.Y), Col);
	m_Buffer.AddVertex(Vertices.Max, Col);
	m_Buffer.AddVertex(Vector2(Vertices.Min.X, Vertices.Max.Y), Col);

	AddTriangleIndices(Offset);
}

void Paint::AddTriangles(const Rect& Vertices, const Rect& TexCoords, const Color& Col, uint32_t Offset)
{
	m_Buffer.AddVertex(Vertices.Min, TexCoords.Min, Col);
	m_Buffer.AddVertex(Vector2(Vertices.Max.X, Vertices.Min.Y), Vector2(TexCoords.Max.X, TexCoords.Min.Y), Col);
	m_Buffer.AddVertex(Vertices.Max, TexCoords.Max, Col);
	m_Buffer.AddVertex(Vector2(Vertices.Min.X, Vertices.Max.Y), Vector2(TexCoords.Min.X, TexCoords.Max.Y), Col);

	AddTriangleIndices(Offset);
}

void Paint::AddTriangleIndices(uint32_t Offset)
{
	m_Buffer.AddIndex(Offset);
	m_Buffer.AddIndex(Offset + 1);
	m_Buffer.AddIndex(Offset + 2);
	m_Buffer.AddIndex(Offset);
	m_Buffer.AddIndex(Offset + 2);
	m_Buffer.AddIndex(Offset + 3);
}

DrawCommand& Paint::PushCommand(uint32_t IndexCount, uint32_t TextureID)
{
	return m_Buffer.PushCommand(IndexCount, TextureID, !m_ClipStack.empty() ? m_ClipStack.back() : Rect());
}

}