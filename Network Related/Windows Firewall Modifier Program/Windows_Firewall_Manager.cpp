#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <random>
#include <Windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netfw.h>
#include <comdef.h>
#include <NetCon.h>
#include <thread>
#include <chrono>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

std::vector<std::string> remoteIpList;

// Function to convert BSTR to std::string
std::string convert_bstr_2_string(BSTR bstr) {
    if (!bstr)
        return "";
    int len = SysStringLen(bstr);
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, bstr, len, NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, bstr, len, &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring convert_string_2_wstring(std::string narrowString) {
    int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, narrowString.c_str(), -1, nullptr, 0);
    wchar_t* wideStringBuffer = new wchar_t[wideStrLength];
    MultiByteToWideChar(CP_UTF8, 0, narrowString.c_str(), -1, wideStringBuffer, wideStrLength);
    std::wstring wideString(wideStringBuffer);
    delete[] wideStringBuffer;
    return wideString;
}



short check_fw_rule_exist(std::string ruleNameStr) {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed." << std::endl;
        return 2;
    }

    // Create instance of the firewall policy
    INetFwPolicy2* fwPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 object." << std::endl;
        CoUninitialize();
        return 2;
    }

    // Get rules collection
    INetFwRules* fwRules = nullptr;
    hr = fwPolicy->get_Rules(&fwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get INetFwRules collection." << std::endl;
        fwPolicy->Release();
        CoUninitialize();
        return 2;
    }

    // Find the rule by name
    INetFwRule* firewallRule = nullptr;
    BSTR ruleName = SysAllocString(convert_string_2_wstring(ruleNameStr).c_str());
    hr = fwRules->Item(ruleName, &firewallRule);
    SysFreeString(ruleName);
    if (FAILED(hr)) {
        fwRules->Release();
        fwPolicy->Release();
        CoUninitialize();
        return 0;
    }
    fwRules->Release();
    fwPolicy->Release();
    CoUninitialize();
    return 1;
}

short create_fw_rule(std::string ruleNameStr, std::string ruleDescStr, std::string ruleActStr) {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed." << std::endl;
        system("pause");
        return 1;
    }

    // Create instance of the firewall policy
    INetFwPolicy2* fwPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 object." << std::endl;
        CoUninitialize();
        system("pause");
        return 1;
    }

    // Get rules collection
    INetFwRules* fwRules = nullptr;
    hr = fwPolicy->get_Rules(&fwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get INetFwRules collection." << std::endl;
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return 1;
    }

    // Create a new rule with set properties
    INetFwRule* newFirewallRule = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&newFirewallRule);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwRule object." << std::endl;
        fwRules->Release();
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return 1;
    }


    // Set properties for the new rule
    BSTR bstrBuffer = _bstr_t(convert_string_2_wstring(ruleNameStr).c_str());
    newFirewallRule->put_Name(bstrBuffer);
    SysFreeString(bstrBuffer);
    bstrBuffer = _bstr_t(convert_string_2_wstring(ruleDescStr).c_str());
    newFirewallRule->put_Description(bstrBuffer);
    SysFreeString(bstrBuffer);
    BSTR service = _bstr_t(L"C:\\Windows\\System32\\svchost.exe");
    newFirewallRule->put_ApplicationName(service);
    if (ruleActStr == "a") {
        newFirewallRule->put_Action(NET_FW_ACTION_ALLOW);
    }
    else if (ruleActStr == "b") {
        newFirewallRule->put_Action(NET_FW_ACTION_BLOCK);
    }
    newFirewallRule->put_Direction(NET_FW_RULE_DIR_IN);
    newFirewallRule->put_Enabled(VARIANT_TRUE);

    // Add the new rule to the collection
    fwRules->Add(newFirewallRule);

    // Release resources
    SysFreeString(service);
    newFirewallRule->Release();
    fwRules->Release();
    fwPolicy->Release();
    CoUninitialize();

    return 0;
}

void update_fw_rule(std::string ruleNameStr) {
    std::string ip_list_format_buffer;
    if (remoteIpList.size() != 0) {
        for (size_t i = 0; i < remoteIpList.size(); ++i) {
            ip_list_format_buffer += "," + remoteIpList[i];
        }
        if (!ip_list_format_buffer.empty()) {
            ip_list_format_buffer = ip_list_format_buffer.substr(1);
        }
        else {
            return;
        }
    }
    else {
        ip_list_format_buffer = "";
    }

    // Initialize COM library
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed." << std::endl;
        system("pause");
        return;
    }

    // Create instance of the firewall policy
    INetFwPolicy2* fwPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 object." << std::endl;
        CoUninitialize();
        system("pause");
        return;
    }

    // Get rules collection
    INetFwRules* fwRules = nullptr;
    hr = fwPolicy->get_Rules(&fwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get INetFwRules collection." << std::endl;
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return;
    }

    // Find the rule by name
    INetFwRule* firewallRule = nullptr;
    BSTR ruleName = SysAllocString(convert_string_2_wstring(ruleNameStr).c_str());
    hr = fwRules->Item(ruleName, &firewallRule);
    SysFreeString(ruleName);

    if (FAILED(hr)) {
        std::cerr << "Failed to get the rule. The rule is not found or exist." << std::endl;
        fwRules->Release();
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return;
    }

    // Overwrite the remote address list with a new one
    firewallRule->put_RemoteAddresses(_bstr_t(convert_string_2_wstring(ip_list_format_buffer).c_str()));

    // Renew the remote list variable
    if (ip_list_format_buffer != "") {
        BSTR currentRemoteIpAddress;
        firewallRule->get_RemoteAddresses(&currentRemoteIpAddress);
        std::string currentRemoteIpAddressStr = convert_bstr_2_string(currentRemoteIpAddress);
        std::istringstream tokenStream(currentRemoteIpAddressStr);
        std::string vectorFormat;
        std::string vectorFormat2;
        remoteIpList.clear();
        while (getline(tokenStream, vectorFormat, ',')) {
            std::istringstream iss(vectorFormat);
            while (getline(iss, vectorFormat2, '/')) {
                remoteIpList.push_back(vectorFormat2);
                break;
            }
        }
        SysFreeString(currentRemoteIpAddress);
    }
    firewallRule->Release();
    fwRules->Release();
    fwPolicy->Release();
    CoUninitialize();
}

void reset_fw_rule(std::string ruleNameStr, std::string ruleDescStr, std::string ip_addr) {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed." << std::endl;
        system("pause");
        return;
    }

    // Create instance of the firewall policy
    INetFwPolicy2* fwPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 object." << std::endl;
        CoUninitialize();
        system("pause");
        return;
    }

    // Get rules collection
    INetFwRules* fwRules = nullptr;
    hr = fwPolicy->get_Rules(&fwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get INetFwRules collection." << std::endl;
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return;
    }

    // Find the rule by name
    INetFwRule* firewallRule = nullptr;
    BSTR ruleName = SysAllocString(convert_string_2_wstring(ruleNameStr).c_str());
    hr = fwRules->Item(ruleName, &firewallRule);
    SysFreeString(ruleName);

    if (FAILED(hr)) {
        std::cerr << "Failed to get the rule. The rule is not found or exist." << std::endl;
        fwRules->Release();
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return;
    }

    BSTR name;
    firewallRule->get_Name(&name);
    firewallRule->Release();
    fwRules->Remove(name); // Remove the old rule
    SysFreeString(name);

    // Create a new rule with modified properties
    INetFwRule* newFirewallRule = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&newFirewallRule);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwRule object." << std::endl;
        fwRules->Release();
        fwPolicy->Release();
        CoUninitialize();
        system("pause");
        return;
    }

    BSTR ruleDesc = _bstr_t(convert_string_2_wstring(ruleDescStr).c_str());
    BSTR remoteAddr = _bstr_t(convert_string_2_wstring(ip_addr).c_str());
    BSTR service = _bstr_t(L"C:\\Windows\\System32\\svchost.exe");

    // Set properties for the new rule
    newFirewallRule->put_Action(NET_FW_ACTION_BLOCK);
    newFirewallRule->put_Direction(NET_FW_RULE_DIR_IN);
    newFirewallRule->put_Enabled(VARIANT_TRUE);
    newFirewallRule->put_Name(ruleName);
    newFirewallRule->put_Description(ruleDesc);
    newFirewallRule->put_ApplicationName(service);
    newFirewallRule->put_RemoteAddresses(remoteAddr);

    // Add the new rule to the collection
    fwRules->Add(newFirewallRule);

    // Release resources
    newFirewallRule->Release();
    SysFreeString(ruleName);
    SysFreeString(ruleDesc);
    SysFreeString(remoteAddr);
    SysFreeString(service);
    fwRules->Release();
    fwPolicy->Release();
    CoUninitialize();
}

void remove_ip_from_ip_list(std::string ip_addr) {
    std::vector<std::string> ip_list_format_buffer;
    std::string item;
    for (size_t i = 0; i < remoteIpList.size(); ++i) {
        item = remoteIpList[i];
        if (item != ip_addr) {
            ip_list_format_buffer.push_back(item);
        }
    }
    remoteIpList.clear();
    if (!ip_list_format_buffer.empty()) {
        remoteIpList = ip_list_format_buffer;
    }
}


int main() {
    std::string userInput;
    bool helpInfo = false;
    std::string ip_addr;
    size_t validate;
    std::string userAction;
    std::string ruleName;
    std::string ruleDesc;
    std::string ruleAct;
    std::cout << "# Note, firewall rule name mustn't have numeric character and exceed 8 characters.\n\n";
    std::cout << "Please enter firewall rule name: ";
    std::cin >> ruleName;
    short checker = check_fw_rule_exist(ruleName);
    if (checker == 0) {
        std::cout << "\nPlease enter firewall rule description: ";
        std::cin >> ruleDesc;
        std::cout << "\nRule Action [ (a)llow / (b)lock ]: ";
        std::cin >> ruleAct;
        create_fw_rule(ruleName, ruleDesc, ruleAct);
        system("pause");
        if (ruleAct == "a") {
            ruleAct = "Allow connection";
        }
        else if (ruleAct == "b") {
            ruleAct = "Block connection";
        }
    }
    else if (checker == 1){
        std::cout << "\n\nRule already exist...\n";
        system("pause");
        return 0;
    }
    else {
        std::cout << "Couldn't validate firewall rule, is the administrator permission were given?";
        system("pause");
        return 0;
    }
    
    while (true) {
        system("cls");
        std::cout << "Firewall properties:\n";
        std::cout << "Name: " + ruleName << std::endl;
        std::cout << "Description: " + ruleDesc << std::endl;
        std::cout << "Action: " + ruleAct << std::endl;
        if (helpInfo) {
            std::cout << "\nTo add remote IP address:\n\tadd 192.168.1.1\n";
            std::cout << "\nTo delete remote IP address:\n\tdel 192.168.1.1\n";
            helpInfo = false;
        }
        std::cout << "\nCurrent remote IP address in the rule:\n";
        if (remoteIpList.size() > 0) {
            for (size_t i = 0; i < remoteIpList.size(); ++i) {
                std::cout << i + 1 << ") " << remoteIpList[i] << std::endl;
            }
        }
        else {
            std::cout << "None\n";
        }
        std::cout << "\nCommand information can be find in HELP command\n";
        std::cout << "> ";
        std::getline(std::cin, userInput);
        if (userInput == "HELP" || userInput == "help" || userInput == "h") {
            helpInfo = true;
        }
        else {
            validate = userInput.find(" ");
            if (validate != std::string::npos) {
                std::istringstream iss(userInput);
                iss >> userAction >> ip_addr;
                if (userAction == "add") {
                    remoteIpList.push_back(ip_addr);
                }
                else if (userAction == "del") {
                    remove_ip_from_ip_list(ip_addr);
                }
                update_fw_rule(ruleName);
            }
        }
    }
}