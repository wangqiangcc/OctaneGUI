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

#include "Table.h"
#include "../Assert.h"
#include "../Json.h"
#include "../Paint.h"
#include "../String.h"
#include "../ThemeProperties.h"
#include "HorizontalContainer.h"
#include "Splitter.h"
#include "Text.h"
#include "VerticalContainer.h"

namespace OctaneGUI
{

//
// TableCell
//

class TableCell : public Container
{
    CLASS(TableCell)

public:
    TableCell(Window* InWindow)
        : Container(InWindow)
    {
        SetClip(true);
    }

    virtual Vector2 DesiredSize() const override
    {
        return { GetSize().X, ChildrenSize().Y };
    }
};

//
// TableRow
//

class TableRow : public HorizontalContainer
{
    CLASS(TableRow)

public:
    TableRow(Window* InWindow)
        : HorizontalContainer(InWindow)
    {
        SetSpacing({});
        SetExpand(Expand::Width);
    }

    std::shared_ptr<Container> AddCell()
    {
        if (!m_Cells.empty())
        {
            Cell& Back = m_Cells.back();
            Back.Separator = AddControl<Container>();
            Back.Separator->SetExpand(Expand::Height);
        }

        std::shared_ptr<TableCell> Result = AddControl<TableCell>();
        m_Cells.push_back({ Result, nullptr });
        return Result;
    }

    TableRow& SetCellSize(size_t Index, float Width, float SeparatorWidth)
    {
        const Cell& Item = GetCell(Index);

        const Vector2 ContentsSize = Item.Contents->ChildrenSize();
        Item.Contents->SetSize({ Width, ContentsSize.Y });

        if (Item.Separator)
        {
            Item.Separator->SetSize({ SeparatorWidth, 0.0f });
        }

        return *this;
    }

    size_t Cells() const
    {
        return m_Cells.size();
    }

    std::shared_ptr<Container> GetCellContainer(size_t Index) const
    {
        return GetCell(Index).Contents;
    }

private:
    struct Cell
    {
    public:
        std::shared_ptr<TableCell> Contents { nullptr };
        std::shared_ptr<Container> Separator { nullptr };
    };

    const Cell& GetCell(size_t Index) const
    {
        Assert(Index < m_Cells.size(), "Invalid column given %zu! Number of columns: %zu.\n", Index, m_Cells.size());
        return m_Cells[Index];
    }

    std::vector<Cell> m_Cells {};
};

//
// TableRows
//

class TableRows : public VerticalContainer
{
    CLASS(TableRows)

public:
    TableRows(Window* InWindow)
        : VerticalContainer(InWindow)
    {
        SetSpacing({});
        SetExpand(Expand::Width);
    }

    std::shared_ptr<TableRow> AddRow()
    {
        std::shared_ptr<TableRow> Result = AddControl<TableRow>();
        return Result;
    }

    std::shared_ptr<TableRow> Row(size_t Index) const
    {
        Assert(Index < NumControls(), "Invalid row given %zu! Number of rows: %zu.\n", Index, NumControls());
        return std::static_pointer_cast<TableRow>(Get(Index));
    }

    size_t Rows() const
    {
        return NumControls();
    }
};

//
// Table
//

Table::Table(Window* InWindow)
    : Container(InWindow)
{
    m_Contents = AddControl<VerticalContainer>();
    m_Contents
        ->SetSpacing({})
        ->SetExpand(Expand::Both);

    m_Header = m_Contents->AddControl<Splitter>();
    m_Header
        ->SetOrientation(Orientation::Vertical)
        .SetOnResized([this](Splitter&) -> void
            {
                SyncSize();
            })
        .SetExpand(Expand::Width);

    m_Rows = m_Contents->AddControl<TableRows>();

    m_Interaction = AddControl<Control>();
    m_Interaction->SetForwardMouseEvents(true);
}

Table& Table::AddColumn(const char32_t* Label)
{
    const std::shared_ptr<Container>& Column = m_Header->AddContainer();
    Column->SetClip(true);
    const std::shared_ptr<Text> TextComponent = Column->AddControl<Text>();
    TextComponent->SetText(Label);
    return *this;
}

Table& Table::FitColumn(size_t Column)
{
    const std::shared_ptr<Container>& Heading = m_Header->GetSplit(Column);

    float Width = Heading->GetSize().X;
    for (size_t Row = 0; Row < Rows(); Row++)
    {
        const std::shared_ptr<Container> Cell = this->Cell(Row, Column);
        const Vector2 Size = Cell->ChildrenSize();
        Width = std::max<float>(Width, Size.X);
    }

    m_Header->SetSplitterWidth(Column, Width);

    return *this;
}

size_t Table::Columns() const
{
    return m_Header->Count();
}

Table& Table::AddRow()
{
    const size_t Index = m_Rows->Rows();
    std::shared_ptr<TableRow> Row = m_Rows->AddRow();

    for (size_t I = 0; I < m_Header->Count(); I++)
    {
        Row->AddCell();
    }

    SyncSize(Index);

    return *this;
}

Table& Table::ClearRows()
{
    m_Rows->ClearControls();
    m_Selected = -1;
    return *this;
}

size_t Table::Rows() const
{
    return m_Rows->NumControls();
}

Table& Table::SetRowSelectable(bool Value)
{
    m_RowSelectable = Value;
    return *this;
}

bool Table::RowSelectable() const
{
    return m_RowSelectable;
}

std::shared_ptr<Container> Table::Cell(size_t Row, size_t Column) const
{
    std::shared_ptr<TableRow> RowContainer = m_Rows->Row(Row);
    return RowContainer->GetCellContainer(Column);
}

Table& Table::SetOnSelected(OnSelectedSignature&& Fn)
{
    m_OnSelected = std::move(Fn);
    return *this;
}

std::weak_ptr<Control> Table::GetControl(const Vector2& Point) const
{
    std::weak_ptr<Control> Result = Container::GetControl(Point);
    if (!Result.expired() && m_Header->HasControl(Result.lock()))
    {
        return Result;
    }

    if (Contains(Point))
    {
        Result = m_Interaction;
    }

    return Result;
}

Vector2 Table::DesiredSize() const
{
    return m_Contents->DesiredSize();
}

void Table::OnPaint(Paint& Brush) const
{
    OnPaintSelection(Brush, (size_t)m_Hovered);
    if (m_Hovered != m_Selected)
    {
        OnPaintSelection(Brush, (size_t)m_Selected);
    }

    Container::OnPaint(Brush);
}

void Table::OnLoad(const Json& Root)
{
    Container::OnLoad(Root);

    const Json& Columns = Root["Header"];
    Columns.ForEach([this](const Json& Item) -> void
        {
            AddColumn(String::ToUTF32(Item["Label"].String()).c_str());
        });

    const Json& Rows = Root["Rows"];
    for (unsigned int RowIdx = 0; RowIdx < Rows.Count(); RowIdx++)
    {
        AddRow();

        const Json& Row = Rows[RowIdx];
        const Json& RowColumns = Row["Columns"];
        for (unsigned int ColumnIdx = 0; ColumnIdx < RowColumns.Count(); ColumnIdx++)
        {
            const Json& Column = RowColumns[ColumnIdx];
            std::shared_ptr<Container> Cell = this->Cell(RowIdx, ColumnIdx);
            Cell->OnLoad(Column);
        }
    }

    SetRowSelectable(Root["RowSelectable"].Boolean(RowSelectable()));

    SyncSize();
}

void Table::OnMouseMove(const Vector2& Position)
{
    if (!m_RowSelectable)
    {
        return;
    }

    int32_t Hovered { -1 };
    for (size_t I = 0; I < m_Rows->Rows(); I++)
    {
        const std::shared_ptr<Control>& Row = m_Rows->Get(I);

        if (Row->Contains(Position))
        {
            Hovered = (int32_t)I;
            break;
        }
    }

    if (m_Hovered != Hovered)
    {
        m_Hovered = Hovered;
        Invalidate(InvalidateType::Paint);
    }
}

bool Table::OnMousePressed(const Vector2&, Mouse::Button Button, Mouse::Count)
{
    if (Button == Mouse::Button::Left)
    {
        if (m_Hovered != -1)
        {
            if (m_Selected != m_Hovered)
            {
                m_Selected = m_Hovered;
                Invalidate(InvalidateType::Paint);

                if (m_OnSelected)
                {
                    m_OnSelected(*this, (size_t)m_Selected);
                }
            }
        }
    }

    return false;
}

void Table::OnMouseLeave()
{
    m_Hovered = -1;
    Invalidate(InvalidateType::Paint);
}

void Table::SyncSize()
{
    for (size_t Row = 0; Row < m_Rows->Rows(); Row++)
    {
        SyncSize(Row);
    }
}

void Table::SyncSize(size_t Row)
{
    const Vector2 SplitterSize { m_Header->SplitterSize() };

    std::shared_ptr<TableRow> RowContainer = m_Rows->Row(Row);
    for (size_t Column = 0; Column < RowContainer->Cells(); Column++)
    {
        const std::shared_ptr<Container>& Header = m_Header->GetSplit(Column);
        RowContainer->SetCellSize(Column, Header->GetSize().X, SplitterSize.X);
    }
}

void Table::OnPaintSelection(Paint& Brush, size_t Index) const
{
    if (Index >= 0 && Index < m_Rows->Rows())
    {
        Color Background { GetProperty(ThemeProperties::TextSelectable_Hovered).ToColor() };
        Background.A = 128;
        const std::shared_ptr<Control>& Row = m_Rows->Get(Index);
        Brush.Rectangle(Row->GetAbsoluteBounds(), Background);
    }
}

}
