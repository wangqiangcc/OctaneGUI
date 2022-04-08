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

#include "Menu.h"
#include "../Json.h"
#include "../Paint.h"
#include "MenuItem.h"
#include "Panel.h"
#include "Separator.h"
#include "TextSelectable.h"
#include "VerticalContainer.h"

namespace OctaneGUI
{

Menu::Menu(Window* InWindow)
	: Container(InWindow)
{
	m_Panel = AddControl<Panel>();
	m_Panel->SetExpand(Expand::Both);

	m_Container = AddControl<VerticalContainer>();
	m_Container
		->SetSpacing({ 0.0f, 0.0f })
		->SetExpand(Expand::Width);
}

Menu& Menu::AddItem(const char* InText, OnEmptySignature Fn)
{
	std::shared_ptr<MenuItem> Item = std::make_shared<MenuItem>(GetWindow());
	Item
		->SetOnHovered([this](const TextSelectable& Item) -> void
			{
				OnHovered(static_cast<const MenuItem&>(Item));
			})
		.SetOnPressed([this](const TextSelectable& Item) -> void
			{
				OnSelected(static_cast<const MenuItem&>(Item));
			})
		.SetText(InText)
		.SetExpand(Expand::Width);

	m_Container->InsertControl(Item);

	m_Items.push_back(Item);
	m_Callbacks[Item.get()] = Fn;
	Resize();

	return *this;
}

std::shared_ptr<MenuItem> Menu::GetItem(const char* InText) const
{
	std::shared_ptr<MenuItem> Result;

	const std::string Value = InText;
	for (const std::shared_ptr<MenuItem>& Item : m_Items)
	{
		if (Value == Item->GetText())
		{
			Result = Item;
			break;
		}
	}

	return Result;
}

Menu& Menu::AddSeparator()
{
	std::shared_ptr<Separator> Item = std::make_shared<Separator>(GetWindow());
	Item->SetOnHover([this](const Control&) -> void
		{
			if (m_Menu)
			{
				SetSelected(m_Menu, false);
				RemoveControl(m_Menu);
				m_Menu = nullptr;
			}
		});
	m_Container->InsertControl(Item);
	Resize();
	return *this;
}

Menu& Menu::Close()
{
	if (m_Menu)
	{
		m_Menu->Close();
		RemoveControl(m_Menu);
	}

	for (const std::shared_ptr<MenuItem>& Item : m_Items)
	{
		Item->SetSelected(false);
	}

	m_Menu = nullptr;
	return *this;
}

void Menu::GetMenuItems(std::vector<std::shared_ptr<MenuItem>>& Items) const
{
	Items.insert(Items.end(), m_Items.begin(), m_Items.end());

	for (const std::shared_ptr<MenuItem>& Item : m_Items)
	{
		const std::shared_ptr<Menu> ItemMenu = Item->GetMenu();

		if (ItemMenu)
		{
			ItemMenu->GetMenuItems(Items);
		}
	}
}

void Menu::OnLoad(const Json& Root)
{
	Container::OnLoad(Root);

	const Json& Items = Root["Items"];
	for (int I = 0; I < Items.Count(); I++)
	{
		const Json& Item = Items[I];

		// TODO: More generic way of adding separator types.
		if (Item["Type"].IsNull())
		{
			AddItem(Item["Text"].String());

			const std::shared_ptr<MenuItem>& MI = m_Items.back();
			MI->OnLoad(Item);
			MI->SetExpand(Expand::Width);
		}
		else if (std::string(Item["Type"].String()) == "Separator")
		{
			AddSeparator();
		}
	}
}

void Menu::Resize()
{
	const Vector2 Margins = GetProperty(ThemeProperties::Menu_Margins).Vector();
	const float RightPadding = GetProperty(ThemeProperties::Menu_RightPadding).Float();

	std::vector<std::shared_ptr<Control>> Controls;
	m_Container->GetControls(Controls);
	Vector2 Size;
	for (const std::shared_ptr<Control>& Item : Controls)
	{
		const Vector2 ItemSize = Item->GetSize();
		Size.X = std::max<float>(Size.X, ItemSize.X);
		Size.Y += ItemSize.Y;
	}

	m_Container->SetPosition(Margins);
	SetSize(Size + Vector2(RightPadding, Margins.Y * 2.0f));
}

void Menu::OnHovered(const MenuItem& Item)
{
	if (m_Menu && Item.GetMenu() == m_Menu)
	{
		return;
	}

	SetSelected(m_Menu, false);
	RemoveControl(m_Menu);

	m_Menu = Item.GetMenu();
	if (!m_Menu)
	{
		return;
	}

	SetSelected(m_Menu, true);

	const Vector2 Margins = GetProperty(ThemeProperties::Menu_Margins).Vector();
	const Vector2 Position = Item.GetPosition() + Vector2(GetSize().X, 0.0f);
	m_Menu->SetPosition(Position);
	InsertControl(m_Menu);
}

void Menu::OnSelected(const MenuItem& Item)
{
	if (Item.GetMenu())
	{
		return;
	}

	if (m_Callbacks.find(&Item) == m_Callbacks.end())
	{
		return;
	}

	OnEmptySignature Fn = m_Callbacks[&Item];

	if (Fn != nullptr)
	{
		Fn();
	}
}

void Menu::SetSelected(const std::shared_ptr<Menu>& InMenu, bool Selected) const
{
	if (!InMenu)
	{
		return;
	}

	for (const std::shared_ptr<MenuItem>& Item : m_Items)
	{
		if (Item->GetMenu() == InMenu)
		{
			Item->SetSelected(Selected);
			break;
		}
	}
}

}