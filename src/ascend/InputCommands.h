#pragma once

struct InputCommands
{
	void Reset() {
		horizontalZ = 0.0f;
		horizontalX = 0.0f;
		vertical = 0.0f;

	}

	float horizontalZ;
	float horizontalX;
	float vertical;
	int mouseX;
	int mouseY;
	bool mouseLButtonDown;
	bool mouseRButtonDown;

};
