#include "GraphicsUserInterfaceModule.h"
#include <ascend/UserInterface/imgui/imgui.h>
ImGuiGraphicsUserInterfaceModule::~ImGuiGraphicsUserInterfaceModule()
{

}

void ImGuiGraphicsUserInterfaceModule::Initialize()
{
	bool yip = true;
	ImGui::Checkbox("test", &yip);
}