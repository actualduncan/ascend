#include <ascend/UserInterface/UserInterfaceModuleInterface.h>

class ImGuiGraphicsUserInterfaceModule : public ImGuiUserInterfaceModule
{
public:
	ImGuiGraphicsUserInterfaceModule();
	~ImGuiGraphicsUserInterfaceModule();
	void Initialize() override;
private:
};