class ImGuiUserInterfaceModule
{
protected:
	bool m_isEnabled;
public:
	ImGuiUserInterfaceModule(bool Enabled) : m_isEnabled(true) {};
	virtual ~ImGuiUserInterfaceModule() {};
	virtual void Initialize() = 0;
	bool IsEnabled() { return m_isEnabled; }
};