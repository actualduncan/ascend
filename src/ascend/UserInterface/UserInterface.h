#include "imgui/imgui.h"
#include <iostream>
#include <vector>

class ImGuiUserInterfaceModule;

class ImGuiUserInterface
{
public:
	ImGuiUserInterface();
	~ImGuiUserInterface();

	void Initialize();
private:
	std::vector<ImGuiUserInterfaceModule*> UserInterfaceModules;
};

