/*=========================================================
SPIRVShader.h - Abstraction class for binary compiled SPIRV
shaders from GLSL code. Requires GLSLlang library to be
integrated before implementation.
==========================================================*/

#pragma once
#include <vector>
#include <fstream>
#include <vulkan\vulkan.h>

namespace Vulkan 
{

	class SPIRVShader
	{
	public:
		SPIRVShader(std::vector<char> vertCode, std::vector<char> fragCode)
		{
			m_vertShaderCode = vertCode;
			m_fragShaderCode = fragCode;
		};
		~SPIRVShader() {};
		std::vector<char> GetVertCode();
		std::vector<char> GetFragCode();
		
		static std::vector<char> ReadBinaryFile(const std::string& filename) {
			std::ifstream f(filename, std::ios::ate | std::ios::binary);

			if (!f.is_open()) {
				throw std::runtime_error("failed to open file!");
			}

			size_t fileSize = (size_t)f.tellg();
			std::vector<char> buffer(fileSize);

			f.seekg(0);
			f.read(buffer.data(), fileSize);
			f.close();

			return buffer;
		}


	private:
		std::vector<char> m_vertShaderCode;
		std::vector<char> m_fragShaderCode;
	};
}