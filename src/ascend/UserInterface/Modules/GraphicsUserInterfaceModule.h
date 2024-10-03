#include <ascend/UserInterface/UserInterfaceModuleInterface.h>
// User Interface Module for all Graphics
// this should be the entry point for all IMGUI related items that refer to graphics techniques
//
class ImGuiGraphicsUserInterfaceModule : public ImGuiUserInterfaceModule
{
public:
	ImGuiGraphicsUserInterfaceModule() : ImGuiUserInterfaceModule(true) {};
	~ImGuiGraphicsUserInterfaceModule();
	void Initialize() override;
private:
};