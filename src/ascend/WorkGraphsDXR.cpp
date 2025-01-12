#include "WorkGraphsDXR.h"
#include "DX12/DX12.h"

WorkGraphsDXR::WorkGraphsDXR()
{

}

WorkGraphsDXR::~WorkGraphsDXR()
{

}

void WorkGraphsDXR::Initialize()
{
	DX12::Initialize();
}

void WorkGraphsDXR::Update(float dt)
{

}

void WorkGraphsDXR::Render()
{
	DX12::StartFrame();

	DX12::EndFrame();
}

void WorkGraphsDXR::ImGUI()
{

}
