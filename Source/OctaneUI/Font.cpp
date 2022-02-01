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

#include "Font.h"
#include "Rect.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include "Texture.h"

#include <cmath>
#include <fstream>

namespace OctaneUI
{

Font::Glyph::Glyph()
	: m_Min()
	, m_Max()
	, m_Offset()
	, m_Advance()
{
}

Font::Font()
	: m_Size(0.0f)
	, m_Ascent(0.0f)
	, m_Descent(0.0f)
	, m_SpaceSize()
	, m_Texture(nullptr)
{
}

Font::~Font()
{
}

bool Font::Load(const char* Path, float Size)
{
	std::ifstream Stream;
	Stream.open(Path, std::ios_base::in | std::ios_base::binary);
	if (!Stream.is_open())
	{
		return false;
	}

	Stream.seekg(0, std::ios_base::end);
	size_t FileSize = Stream.tellg();
	Stream.seekg(0, std::ios_base::beg);

	std::vector<char> Buffer;
	Buffer.resize(FileSize);

	Stream.read(&Buffer[0], Buffer.size());
	Stream.close();

	m_Size = Size;
	Vector2 TextureSize = Vector2(1024.0f, 1024.0f);
	std::vector<uint8_t> Texture;
	Texture.resize((int)TextureSize.X * (int)TextureSize.Y);

	stbtt_bakedchar Glyphs[96];
	int Result = stbtt_BakeFontBitmap((uint8_t*)Buffer.data(), 0, Size, &Texture[0], (int)TextureSize.X, (int)TextureSize.Y, 32, 96, Glyphs);
	if (Result == 0)
	{
		return false;
	}

	float LineGap;
	stbtt_GetScaledFontVMetrics((uint8_t*)Buffer.data(), 0, Size, &m_Ascent, &m_Descent, &LineGap);

	// Convert data to RGBA32
	std::vector<uint8_t> RGBA32;
	RGBA32.resize((int)TextureSize.X * (int)TextureSize.Y * 4);
	size_t Index = 0;
	for (size_t I = 0; I < Texture.size(); I++)
	{
		RGBA32[Index] = 255;
		RGBA32[Index + 1] = 255;
		RGBA32[Index + 2] = 255;
		RGBA32[Index + 3] = Texture[I];
		Index += 4;
	}

	m_Texture = Texture::Load(RGBA32, (uint32_t)TextureSize.X, (uint32_t)TextureSize.Y);
	if (!m_Texture)
	{
		return false;
	}

	for (int I = 0; I < 96; I++)
	{
		const stbtt_bakedchar& Char = Glyphs[I];

		Glyph Item;
		Item.m_Min = Vector2(Char.x0, Char.y0);
		Item.m_Max = Vector2(Char.x1, Char.y1);
		Item.m_Offset = Vector2(Char.xoff, Char.yoff);
		Item.m_Advance = Vector2(Char.xadvance, 0.0f);
		m_Glyphs.push_back(Item);
	}

	m_SpaceSize = Measure(" ");

	return true;
}

bool Font::Draw(int32_t Char, Vector2& Position, Rect& Vertices, Rect& TexCoords, bool NormalizeTexCoords) const
{
	if (Char < 0 || Char >= m_Glyphs.size())
	{
		return false;
	}

	const Glyph& Item = m_Glyphs[Char];
	const Vector2 ItemSize = Item.m_Max - Item.m_Min;
	const Vector2 InvertedSize = m_Texture->GetSize().Invert();

	int X = (int)floor(Position.X + Item.m_Offset.X + 0.5f);
	int Y = (int)floor(Position.Y + Item.m_Offset.Y + m_Ascent + m_Descent + 0.5f);

	Vertices.Min = Vector2((float)X, (float)Y);
	Vertices.Max = Vertices.Min + ItemSize;

	if (NormalizeTexCoords)
	{
		TexCoords.Min = Item.m_Min * InvertedSize;
		TexCoords.Max = Item.m_Max * InvertedSize;
	}
	else
	{
		TexCoords.Min = Item.m_Min;
		TexCoords.Max = Item.m_Max;
	}

	Position += Item.m_Advance;
	
	return true;
}

Vector2 Font::Measure(const std::string& Text) const
{
	Vector2 Result;

	Vector2 Position;
	for (char Ch : Text)
	{
		int32_t CodePoint = Ch - 32;
		Rect Vertices, TexCoords;
		Draw(CodePoint, Position, Vertices, TexCoords);
		Result.Y = std::max<float>(Result.Y, Vertices.Height());
	}

	Result.X = Position.X;

	return Result;
}

uint32_t Font::GetID() const
{
	if (!m_Texture)
	{
		return 0;
	}

	return m_Texture->GetID();
}

float Font::GetSize() const
{
	return m_Size;
}

Vector2 Font::GetSpaceSize() const
{
	return m_SpaceSize;
}

}
