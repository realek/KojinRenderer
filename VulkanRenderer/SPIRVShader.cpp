#include "SPIRVShader.h"

std::vector<char> Vulkan::SPIRVShader::GetVertCode()
{
	return m_vertShaderCode;
}

std::vector<char> Vulkan::SPIRVShader::GetFragCode()
{
	return m_fragShaderCode;
}
