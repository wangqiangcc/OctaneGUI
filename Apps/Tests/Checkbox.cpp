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

#include "OctaneUI/OctaneUI.h"
#include "TestSuite.h"

namespace Tests
{

static std::string CheckboxJson(const char* ID, const char* Text, bool TriState)
{
	const char* Json = "{\"Width\": 1280, \"Height\": 720, \"Body\": {\"Controls\": [";

	return std::string(Json)
		+ "{\"Type\": \"Checkbox\", \"ID\": \"" + ID + "\", "
		+ "\"TriState\": " + (TriState ? "true" : "false") + ", "
		+ "\"Text\": {\"Text\": \"" + Text + "\"}}"
		+ "]}}";
}

TEST_SUITE(Checkbox,

TEST_CASE(TwoState,
{
	OctaneUI::ControlList List;
	Application.GetMainWindow()->Load(CheckboxJson("Checkbox", "Checkbox", false).c_str(), List);
	Application.GetMainWindow()->Update();

	std::shared_ptr<OctaneUI::Checkbox> Checkbox = List.To<OctaneUI::Checkbox>("Checkbox");

	const OctaneUI::Vector2 Position = Checkbox->GetAbsoluteBounds().GetCenter();
	Application.GetMainWindow()->OnMouseMove(Position);
	Application.GetMainWindow()->OnMousePressed(Position, OctaneUI::Mouse::Button::Left);
	Application.GetMainWindow()->OnMouseReleased(Position, OctaneUI::Mouse::Button::Left);

	VERIFYF(Checkbox->GetState() == OctaneUI::Checkbox::State::Checked, "TwoState: Checkbox is not checked!");

	Application.GetMainWindow()->OnMousePressed(Position, OctaneUI::Mouse::Button::Left);
	Application.GetMainWindow()->OnMouseReleased(Position, OctaneUI::Mouse::Button::Left);

	return Checkbox->GetState() == OctaneUI::Checkbox::State::None;
})

TEST_CASE(TriState,
{
	OctaneUI::ControlList List;
	Application.GetMainWindow()->Load(CheckboxJson("Checkbox", "Checkbox", true).c_str(), List);
	Application.GetMainWindow()->Update();

	std::shared_ptr<OctaneUI::Checkbox> Checkbox = List.To<OctaneUI::Checkbox>("Checkbox");

	const OctaneUI::Vector2 Position = Checkbox->GetAbsoluteBounds().GetCenter();
	Application.GetMainWindow()->OnMouseMove(Position);
	Application.GetMainWindow()->OnMousePressed(Position, OctaneUI::Mouse::Button::Left);
	Application.GetMainWindow()->OnMouseReleased(Position, OctaneUI::Mouse::Button::Left);

	VERIFYF(Checkbox->GetState() == OctaneUI::Checkbox::State::Intermediate, "TriState: Checkbox is not intermediate!\n");

	Application.GetMainWindow()->OnMousePressed(Position, OctaneUI::Mouse::Button::Left);
	Application.GetMainWindow()->OnMouseReleased(Position, OctaneUI::Mouse::Button::Left);

	VERIFYF(Checkbox->GetState() == OctaneUI::Checkbox::State::Checked, "TriState: Checkbox is not checked!\n");

	Application.GetMainWindow()->OnMousePressed(Position, OctaneUI::Mouse::Button::Left);
	Application.GetMainWindow()->OnMouseReleased(Position, OctaneUI::Mouse::Button::Left);

	return Checkbox->GetState() == OctaneUI::Checkbox::State::None;
})

)

}
