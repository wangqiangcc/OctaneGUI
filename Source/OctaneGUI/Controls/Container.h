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

#include "../VertexBuffer.h"
#include "Control.h"

#include <memory>
#include <vector>

namespace OctaneGUI
{

class Container : public Control
{
	CLASS(Container)

public:
	Container(Window* InWindow);
	virtual ~Container();

	template <class T, typename... TArgs>
	std::shared_ptr<T> AddControl(TArgs... Args)
	{
		std::shared_ptr<T> Result = std::make_shared<T>(GetWindow(), Args...);
		InsertControl(Result);
		return Result;
	}

	std::shared_ptr<Control> CreateControl(const std::string& Type);
	Container* InsertControl(const std::shared_ptr<Control>& Item, int Position = -1);
	Container* RemoveControl(const std::shared_ptr<Control>& Item);
	bool HasControl(const std::shared_ptr<Control>& Item) const;
	void ClearControls();

	Container* Layout();
	void InvalidateLayout();

	virtual std::weak_ptr<Control> GetControl(const Vector2& Point) const;
	// TODO: Rename to GetAllControls.
	void GetControls(std::vector<std::shared_ptr<Control>>& Controls) const;
	const std::vector<std::shared_ptr<Control>>& Controls() const;
	virtual Vector2 DesiredSize() const;

	virtual void OnPaint(Paint& Brush) const override;
	virtual void OnLoad(const Json& Root) override;
	virtual void OnThemeLoaded() override;

protected:
	virtual void PlaceControls(const std::vector<std::shared_ptr<Control>>& Controls) const;

private:
	std::vector<std::shared_ptr<Control>> m_Controls;
	bool m_InLayout { false };
};

}