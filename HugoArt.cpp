#include "HugoArt.h"
#include <iostream>
using namespace std;
const vector<wstring> hugo_progs_arts[] = {
{
L"¨€¨€¨[  ¨€¨€¨[¨€¨€¨[   ¨€¨€¨[ ¨€¨€¨€¨€¨€¨€¨[  ¨€¨€¨€¨€¨€¨€¨[ ¨€¨€¨€¨€¨€¨€¨[ ¨€¨€¨€¨€¨€¨€¨[  ¨€¨€¨€¨€¨€¨€¨[  ¨€¨€¨€¨€¨€¨€¨[ ¨€¨€¨€¨€¨€¨€¨€¨[",
L"¨€¨€¨U  ¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨€¨€¨X¨T¨T¨T¨T¨a ¨€¨€¨X¨T¨T¨T¨€¨€¨[¨€¨€¨X¨T¨T¨€¨€¨[¨€¨€¨X¨T¨T¨€¨€¨[¨€¨€¨X¨T¨T¨T¨€¨€¨[¨€¨€¨X¨T¨T¨T¨T¨a ¨€¨€¨X¨T¨T¨T¨T¨a",
L"¨€¨€¨€¨€¨€¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨€¨€¨U  ¨€¨€¨€¨[¨€¨€¨U   ¨€¨€¨U¨€¨€¨€¨€¨€¨€¨X¨a¨€¨€¨€¨€¨€¨€¨X¨a¨€¨€¨U   ¨€¨€¨U¨€¨€¨U  ¨€¨€¨€¨[¨€¨€¨€¨€¨€¨€¨€¨[",
L"¨€¨€¨X¨T¨T¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨€¨€¨X¨T¨T¨T¨a ¨€¨€¨X¨T¨T¨€¨€¨[¨€¨€¨U   ¨€¨€¨U¨€¨€¨U   ¨€¨€¨U¨^¨T¨T¨T¨T¨€¨€¨U",
L"¨€¨€¨U  ¨€¨€¨U¨^¨€¨€¨€¨€¨€¨€¨X¨a¨^¨€¨€¨€¨€¨€¨€¨X¨a¨^¨€¨€¨€¨€¨€¨€¨X¨a¨€¨€¨U     ¨€¨€¨U  ¨€¨€¨U¨^¨€¨€¨€¨€¨€¨€¨X¨a¨^¨€¨€¨€¨€¨€¨€¨X¨a¨€¨€¨€¨€¨€¨€¨€¨U",
L"¨^¨T¨a  ¨^¨T¨a ¨^¨T¨T¨T¨T¨T¨a  ¨^¨T¨T¨T¨T¨T¨a  ¨^¨T¨T¨T¨T¨T¨a ¨^¨T¨a     ¨^¨T¨a  ¨^¨T¨a ¨^¨T¨T¨T¨T¨T¨a  ¨^¨T¨T¨T¨T¨T¨a ¨^¨T¨T¨T¨T¨T¨T¨a"
},{
L"©·©³      ©³©·",
L"©Ç©Ï©·©³©³©·©³©·©§©§©³©·©³©·©³©·©³",
L"©¿©»©»©ß©»©Ï©»©¿©Ç©¿©¿ ©»©¿©»©Ï©¿",
L"     ©¿         ©¿"
} };
const std::vector<std::wstring> HugoArt::GetHugoArtText(int idx)
{
	if (idx >= sizeof(hugo_progs_arts) / sizeof(hugo_progs_arts[0]))return {};
	return std::vector<std::wstring>(hugo_progs_arts[idx]);
}

void HugoArt::PrintArtText(int idx)
{
	auto art=GetHugoArtText(idx);
	for (const wstring& line : art) {
		wcout << line << std::endl;
	}
}
