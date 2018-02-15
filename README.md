# Policy Parser
The Policy Parser project parses the Administrative Templates located in Windows\PolicyDefinitions by getting their name from their corresponding string table (Windows\PolicyDefinitions\en-US) and their registry key from the administrative template. This is useful for dynamic parsing of Windows Local Group Policies, however, I found it an interesting project as the registry keys for group policies aren't entirely documented.

The program will parse each administrative template and then save their policies in "policydefinitions.txt". For convenience, I have placed a policydefinitions.txt from my computer (Windows 10) in this repo for those just looking for static registry keys.

# Features
- Dynamic parsing of group policies
- Gets the policy name from the associated string table
- No dependencies other than [RapidXML](https://sourceforge.net/projects/rapidxml/) (included in project) and the Visual C++ Runtime