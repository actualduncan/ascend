#include "UserInterface.h"
#include "Modules/GraphicsUserInterfaceModule.h"
ImGuiUserInterface::ImGuiUserInterface()
{

}

ImGuiUserInterface::~ImGuiUserInterface()
{

}

void ImGuiUserInterface::Initialize()
{
	UserInterfaceModules.push_back(new ImGuiGraphicsUserInterfaceModule());

	for (auto&& module : UserInterfaceModules)
	{
		module->Initialize();
	}
}