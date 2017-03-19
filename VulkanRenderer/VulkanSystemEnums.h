namespace Vulkan
{
	enum RenderPassType
	{
		SwapchainManaged = 0,
		Secondary_OnScreen_Forward = 1,
		Secondary_Offscreen_Forward_Shadows = 2,
		Secondary_Offscreen_Deffered_Lights = 3,
		Secondary_Offscreen_Deffered_Normal = 4,
		Secondary_Offscreen_Deffered_Shadows = 5,
		RenderPassCount = 6
	};

	enum ShadowmapResolutions
	{
		e512 = 512,
		e1024 = 1024,
		e2048 = 2028,
		e4096 = 4096
	};
}