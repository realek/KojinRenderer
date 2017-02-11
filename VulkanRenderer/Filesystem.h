#pragma once
#include <vector>
#include <fstream>

namespace FileSystem
{
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


}