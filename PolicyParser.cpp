#include "stdafx.h"
#include <Windows.h>
#include "RapidXML\rapidxml.hpp"
#include <vector>
#include <fstream>
#include <regex>

using namespace rapidxml;

class Policy {
public:
	std::string policyname;
	std::string registrykey;

	Policy(std::string tpolicyname, std::string tregistrykey) {
		policyname = tpolicyname;
		registrykey = tregistrykey;
	}
};

class PolicyDefinition {
public:
	std::string name;
	std::vector<Policy> policies;

	PolicyDefinition(std::string tname) {
		name = tname;
	}
};

std::vector<char> getFileContents(std::string filename) {
	std::ifstream policyfile(filename);

	if (!policyfile) {
		printf("Cannot open file!\n");
		return std::vector<char>();
	}

	std::vector<char> fileContent((std::istreambuf_iterator<char>(policyfile)), std::istreambuf_iterator<char>());
	fileContent.push_back('\0');

	return fileContent;
}

std::string getFileName(std::string filename) {
	std::string singleFileName = filename.substr(filename.find_last_of("\\") + 1);
	singleFileName = singleFileName.erase(singleFileName.size() - 5);
	return singleFileName;
}

std::string getPolicyName(std::string filename, const std::string policyname) {
	std::regex policyNameParse("(^\\$\\(string\\.)(.*)(\\))");
	std::smatch match;
	if (!std::regex_search(policyname.begin(), policyname.end(), match, policyNameParse)) {
		printf("Error regex parsing name!\n");
		return 0;
	}

	std::string truePolicyName = match[2];
	std::string path = getenv("windir");
	path += "\\PolicyDefinitions\\en-US\\" + filename + ".adml";

	std::vector<char> stringTableContent = getFileContents(path);
	if (stringTableContent.size() < 1)
		return "";

	xml_document<> stringTableDoc;
	stringTableDoc.parse<0>(&stringTableContent[0]);

	xml_node<>* policyDefinitionResources = stringTableDoc.first_node("policyDefinitionResources");
	if (!policyDefinitionResources) {
		printf("Could not parse 'policyDefinitionResources'!\n");
		return "";
	}

	xml_node<>* resources = policyDefinitionResources->first_node("resources");
	if (!resources) {
		printf("Could not parse 'resources'!\n");
		return "";
	}

	xml_node<>* stringTable = resources->first_node("stringTable");
	if (!resources) {
		printf("Could not parse 'stringTable'!\n");
		return "";
	}

	for (xml_node<> * string = stringTable->first_node("string"); string; string = string->next_sibling())
	{
		if (string->first_attribute("id")->value() == truePolicyName) {
			return string->value();
		}
	}

	return "";
}

std::vector<PolicyDefinition> getPolicyDefinition(std::string fileName, std::vector<PolicyDefinition> originalDefinitions) {
	std::string simpleFileName = getFileName(fileName); // basically just the file name without extension or path

	std::vector<char> fileContents = getFileContents(fileName);
	if (fileContents.size() < 1)
		return originalDefinitions;

	xml_document<> policyDefinition;

	try {
		policyDefinition.parse<0>(&fileContents[0]);
	}
	catch (const rapidxml::parse_error& e) {
		printf("Error parsing '%s'!\n", fileName.c_str());
		return originalDefinitions;
	}

	xml_node<>* policyInfo = policyDefinition.first_node("policyDefinitions");
	if (!policyInfo) {
		printf("Could not parse 'policyDefinitions' in '%s'!\n", fileName.c_str());
		return originalDefinitions;
	}

	xml_node<>* policies = policyInfo->first_node("policies");
	if (!policies) {
		printf("Could not parse 'policies' in '%s'!\n", fileName.c_str());
		return originalDefinitions;
	}

	PolicyDefinition definition(simpleFileName);

	for (xml_node<> * policy = policies->first_node("policy"); policy; policy = policy->next_sibling())
	{
		xml_attribute<char> *key = policy->first_attribute("key");
		if (!key)
			continue;

		xml_attribute<char> *name = policy->first_attribute("name");
		if (!name)
			continue;

		xml_attribute<char> *displayName = policy->first_attribute("displayName");
		if (!displayName)
			continue;

		std::string registryKey = key->value();
		registryKey += "\\";
		registryKey += name->value();

		std::string policyName = getPolicyName(simpleFileName, displayName->value());
		if (policyName.length() == 0)
			continue;

		definition.policies.push_back(Policy(policyName, registryKey));
	}

	originalDefinitions.push_back(definition);

	return originalDefinitions;
}

// https://stackoverflow.com/a/20847429
std::vector<std::string> get_all_files_names_within_folder(std::string folder)
{
	std::vector<std::string> names;
	std::string search_path = folder + "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFileA(search_path.c_str(), (LPWIN32_FIND_DATAA)&fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

int main()
{
	std::vector<PolicyDefinition> originalDefinitions;

	std::string path = getenv("windir");
	path += "\\PolicyDefinitions";

	std::vector<std::string> policyDefinitionFiles = get_all_files_names_within_folder(path);
	int totalPolicies = 0;
	std::string totalString = "";

	for (int i = 0; i < policyDefinitionFiles.size(); i++) {
		std::string fileName = path + "\\" + policyDefinitionFiles[i];

		printf("Policy Definition: %s\n", fileName.c_str());

		originalDefinitions = getPolicyDefinition(fileName, originalDefinitions);

		totalString += originalDefinitions.back().name + " : \n";

		for (int c = 0; c < originalDefinitions.back().policies.size(); c++) {
			Policy policy = originalDefinitions.back().policies[c];

			totalString += "\t- " + policy.policyname + " : " + policy.registrykey + "\n";
		}

		totalString += "\n";

		totalPolicies += originalDefinitions.back().policies.size();
	}

	printf("%i policy definitions found, %i total policies found.\n", originalDefinitions.size(), totalPolicies);

	std::ofstream out("policydefintions.txt");
	out << totalString;
	out.close();

	system("pause");

	return 0;
}