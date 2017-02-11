#include "SPIRVShader.h"

std::vector<char> Vk::SPIRVShader::GetVertCode()
{
	return m_vertShaderCode;
}

std::vector<char> Vk::SPIRVShader::GetFragCode()
{
	return m_fragShaderCode;
}
