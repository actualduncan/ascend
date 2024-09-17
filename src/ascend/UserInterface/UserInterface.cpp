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
	UserInterfaceModules.push_back(std::make_unique<ImGuiGraphicsUserInterfaceModule>());

	for (auto&& module : UserInterfaceModules)
	{
		module->Initialize();
	}
}