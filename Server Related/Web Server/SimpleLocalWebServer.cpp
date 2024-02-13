#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Constants
const int DEFAULT_PORT = 80;
const int BUFFER_SIZE = 4096;
char shutdownCode = '0';

int count_character(std::string s, char c) {
    int result = 0;
    for (int i = 0; i < s.length(); i++)
        if (s[i] == c)
            result++;
    return result;
}

std::string get_file_content(const std::string& fileName) {
    // Read content from a file
    std::ifstream file(fileName);
    std::ostringstream fileContent;
    fileContent << file.rdbuf();
    file.close();
    return fileContent.str();
}

// Note: Content-Type: image/jpeg
std::string response_image_file(const std::string& imageName) {
    std::string imageTemplate;
    std::ifstream imageFile(imageName, std::ios::binary);
    std::ostringstream imageContent;
    imageContent << imageFile.rdbuf();
    imageTemplate = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n";
    imageTemplate += "Content-Length: " + std::to_string(imageContent.str().size()) + "\r\n\r\n";
    imageTemplate += imageContent.str() + "\r\n";
    return imageTemplate;
}

// Just extra function, not using it. self explanatory function
std::string get_client_ip(SOCKET clientSocket) {
    sockaddr_in clientAddr{};
    int clientAddrSize = sizeof(clientAddr);
    if (getpeername(clientSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize) == 0) {
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipAddress, INET_ADDRSTRLEN);
        return ipAddress;
    }
    else {
        std::cerr << "Unable to retrieve client IP address.\n";
        return "No_IP";
    }
}




    // This function could be expanded to handle different URLs and POST data
std::string get_response_for_url(const std::string& url, std::vector<std::string> postList) {
    std::string webpage_template;
    std::string response;
    if (url == "/") {
        webpage_template = get_file_content("templates\\index.html");
    }
    else if (url == "/test") {
        webpage_template = get_file_content("templates\\test.html");
    }
    else if (url == "/local-image.jpg") {
        return response_image_file("images\\image.jpg");
    }
    else if (url == "/fetchHello") {
        return "Hello to you too!";
    }
    else if (url == "/fetchServerCondition") {
        return "Im good! Thank you for asking";
    }
    else if (url == "/shutdownTheSystem") {
        if (postList[0] == "shutdownKey" && postList[1] == "1234567890") {
            return "<p>Are you sure ? </p><br><button onclick = \"shutdownConfirm()\">Yes</button>";
        }
        return "null"; // return this if the request doesnt have shutdown key. This is for security.
    }
    else if (url == "/confirmShutdown") {
        if (postList[0] == "shutdownKey" && postList[1] == "1234567890") {
            shutdownCode = '1';
        }
        return "null"; // never return nothing ("")
    }
    else {
        webpage_template = "Unknown URL";
    }
    std::string responsePrefix = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
    std::string fullResponse = responsePrefix + std::to_string(webpage_template.size()) + "\r\n\r\n" + webpage_template;
    return fullResponse;
}







int main() {
    // Initialize Winsock
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket\n";
        WSACleanup();
        return 1;
    }

    // Bind the socket to an address and port
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "\n\tCreated by NWS8421b\n\tSupervised by Ms. Fara\n\tFeel free to explore!\n\n";
    std::cout << "\tServer listening on port " << std::to_string(DEFAULT_PORT) << "...\n\tBrowse http://localhost/" << std::endl;

    std::vector<std::string> postList;
    size_t size;

    while (true) {
        postList.clear();
        //std::cout << "Waiting for request" << std::endl;
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (shutdownCode == '1') {
            break;
        }

        char buffer[BUFFER_SIZE];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::string newBuffer;
        if (bytesRead > 0) {
            std::string sanitizedBuffer(buffer, bytesRead);
            newBuffer = sanitizedBuffer;
        }

        // Extract the URL and POST data from the request
        std::istringstream request(newBuffer);
        std::string method, url, protocol;
        request >> method >> url >> protocol;
        std::string postData;
        std::string fullContent((std::istreambuf_iterator<char>(request)), std::istreambuf_iterator<char>());
        size_t lastNewlinePos = fullContent.rfind('\n');
        postData = fullContent.substr(lastNewlinePos + 1); // Extract the last line in the request, (The line where the POST data is generally placed)

        // Convert POST data into list
        if (count_character(postData, '=') != 0) {
            postData += "&";
            int indexIteration = 0;
            char splitPoint;
            for (short i = 0; i < (count_character(postData, '=') * 2); i++) {
                if (i % 2 == 0) {
                    splitPoint = '=';
                }
                else {
                    splitPoint = '&';
                }
                postList.push_back(postData.substr(indexIteration, postData.find(splitPoint, indexIteration) - indexIteration));
                indexIteration = postData.find(splitPoint, indexIteration) + 1;
            }
            size = postList.size();
        }

        // Generate the response based on the requested URL and POST data
        std::string response = get_response_for_url(url, postList);

        // Send the response
        send(clientSocket, response.c_str(), response.size(), 0);
        closesocket(clientSocket);
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();

    system("cls");
    std::cout << "\n\tWeb Server shutdown successfully!\n\tHope this experience will help you!\n\n";
    system("pause");

    return 0;
}
