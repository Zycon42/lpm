/**
 * @file main.cpp
 *
 * @author Jan Dušek <xdusek17@stud.fit.vutbr.cz>
 * @date 2013
 *
 * File with main function
 */

// project headers
#include "bitarray.h"
#include "bintrie.h"

// system headers
#ifdef WIN32
// mingw windows system headers

// inet_pton exists only in winvista and later
#include <w32api.h>
#define WINVER WindowsVista
#include <ws2tcpip.h>

#ifndef _MSC_VER
// there's missing inet_pton prototype in mingw
extern "C" {
    int WSAAPI inet_pton(int, const char*, char*);
}
#endif

#else
// linux system headers
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

// c++ stl headers
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cassert>
#include <vector>

using namespace std;


typedef BinaryTrie<sizeof(in_addr), int> Subnet4Dict;
typedef BinaryTrie<sizeof(in6_addr), int> Subnet6Dict;

typedef Subnet4Dict::key_type Subnet4;
typedef Subnet6Dict::key_type Subnet6;

/**
 * Converts Ipv4/Ipv6 address from text to numeric representation.
 * @param src text representation
 * @param dst4 pointer where v4 numeric representation will be stored
 * @param dst6 pointer where v6 numeric representation will be stored
 * @return address familly
 */
int convertAddressToNumeric(const char* src, char* dst4, char* dst6) {
    if (inet_pton(AF_INET, src, dst4) == 1) {
    	return AF_INET;
    } else if(inet_pton(AF_INET6, src, dst6) == 1) {
    	return AF_INET6;
    }
    
    return -1;
}

void parseInputFile(const char* fileName, Subnet4Dict& dict4, Subnet6Dict& dict6) {
    ifstream file(fileName);
    istringstream iss;
    string subnet;
    int as;

    if (!file)
        throw runtime_error("Unable to open input file!");

    while (!file.eof()) {
        file >> subnet >> as;

        if (file.bad())
            throw runtime_error("Some serious error occured while reading input file!");

        size_t delim = subnet.find_last_of('/');
        size_t prefixLen = 0;
        iss.str(subnet.substr(delim + 1));
        iss.clear();
        iss >> prefixLen;
        string prefix = subnet.substr(0, delim);

		Subnet4 addr4;
		Subnet6 addr6;
        int family = convertAddressToNumeric(prefix.c_str(), (char*)addr4.internalStorage(), (char*)addr6.internalStorage());

        if (family == AF_INET && prefixLen <= 32) {
			addr4.setSize(prefixLen);
            dict4[addr4] = as;
        } else if (family == AF_INET6 && prefixLen <= 128) {
			addr6.setSize(prefixLen);
            dict6[addr6] = as;
        } else
            throw runtime_error("Unknown Ip address family");
    }
}

void printAppropriateAs(std::istream& stream, Subnet4Dict& dict4, Subnet6Dict& dict6) {
	// on each stream line is Ipv4/Ipv6 address in text representation which is maximum 39 characters long.
    char lineBuf[64];
    while (stream.getline(lineBuf, 64)) {
		if (stream.fail())
			throw runtime_error("Error while reading input Ip addresses");
	
        Subnet4 addr4;
		Subnet6 addr6;
        int family = convertAddressToNumeric(lineBuf, (char*)addr4.internalStorage(), (char*)addr6.internalStorage());
        try {
            int as;
            if (family == AF_INET) {
				addr4.setSize(32);
                as = dict4.best(addr4);
            } else if (family == AF_INET6) {
				addr6.setSize(128);
                as = dict6.best(addr6);
            } else
                throw runtime_error("Unknown Ip address family");

            cout << as << endl;
        } catch (out_of_range& e) {
            cout << "-" << endl;
        }
    }
}

void printUsageAndHelp() {
    static const char* str =
        "Usage:  lpm -i FILE\n"
        "   FILE input file containing subnets and AS numbers\n\n"
        "Program expects list of IPv4/IPv6 addresses separated by newline at stdin\n"
        "and prints AS numbers corresponding to individual addresses.";

    cout << str << endl;
}

int main(int argc, char** argv) {

    // handle bad input args
    if (argc != 3 || strcmp(argv[1], "-i") != 0) {
        printUsageAndHelp();
        return 1;
    }

#ifdef WIN32
    // init wsa for socket api
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        cerr << "WSAStartup failed: " << ret << endl;
        return 1;
    }
#endif

    Subnet4Dict dict4;
    Subnet6Dict dict6;
    parseInputFile(argv[2], dict4, dict6);

	std::ios_base::sync_with_stdio(false);
    printAppropriateAs(std::cin, dict4, dict6);

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}

