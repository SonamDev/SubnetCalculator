#include <iostream>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <bitset>
#include <fstream>

using namespace std;
using namespace boost::algorithm;

const int CLASS_A_MAX = 16777214;
const int CLASS_B_MAX = 65534;
const int CLASS_C_MAX = 254;

struct Subnet {
    unsigned int s_mask;
    unsigned int nextHop;
    string label;
    int prefix;
};

unsigned int address_to_int(string& input) {
    int address = 0;

    vector<string> splitIp;
    split(splitIp, input, is_any_of("."));

    for(const auto& in : splitIp) {
        if(stoi(in) > 0xFF) {
            return -1;
        }
    }

    address |= (stoul(splitIp[0]) << 24u);
    address |= (stoul(splitIp[1]) << 16u);
    address |= (stoul(splitIp[2]) << 8u);
    address |= (stoul(splitIp[3]));

    return address;
}

int max_hosts(string& address) {
    try {
        vector<string> splitIp;
        split(splitIp, address, is_any_of("."));
        int firstOctet = stoi(splitIp[0]);

        if(firstOctet <= 126) {
            return CLASS_A_MAX;
        }
        if(firstOctet > 129 && firstOctet <= 191) {
            return CLASS_B_MAX;
        }
        if(firstOctet > 191 && firstOctet <= 223) {
            return CLASS_C_MAX;
        }
        return -1;
    } catch (exception& e) {
        return -1;
    }
}

string address_from_int(unsigned int* input) {
    string address;

    unsigned int bitmask = 0b11111111;

    address += to_string((*input & (bitmask << 24u)) >> 24u) + ".";
    address += to_string((*input & (bitmask << 16u)) >> 16u) + ".";
    address += to_string((*input & (bitmask << 8u)) >> 8u) + ".";
    address += to_string((*input & (bitmask)));
    return address;
}

unsigned int calculate_next_hop(const int* hosts);

int main() {

    unsigned int mainIp = 0;
    short mainPrefix;
    int step = 0;
    string currentMessage = "Main Network IP > ";
    vector<Subnet> subnets;
    int maxHosts = 0;

    bool multiple = false;

    cout << "\x1B[2J\x1B[H" << endl << flush;
    cout << "Subnet Calculator by Daniel Crespo @ 2019 | Cisco" << endl << endl;

    while(true) {

        cout << currentMessage;

        string str;
        cin >> str;

        if(str == "end") {
            break;
        }

        switch(step) {
            case 0: {
                bool matches = regex_match(str, regex("\\d+\\.\\d+\\.\\d+\\.\\d+\\/\\d+"));

                if(matches) {

                    vector<string> mainNetworkSplit;
                    split(mainNetworkSplit, str, is_any_of("/"));

                    string& address = mainNetworkSplit[0];
                    short prefix = stoi(mainNetworkSplit[1]);

                    maxHosts = max_hosts(address);

                    if(maxHosts == -1) {
                        cout << "\"" << address << "\"" << " is not a valid IP address." << endl;
                        continue;
                    }

                    if(prefix > 30) {
                        cout << "Prefix too long, let at least 2 bytes for hosts" << endl;
                        continue;
                    }

                    mainPrefix = prefix;
                    mainIp = address_to_int(address);

                    if(mainIp == -1) {
                        cout << "\"" << address << "\"" << " is not a valid IP address." << endl;
                        continue;
                    }

                    int bytesToSkip = 32 - prefix;

                    unsigned int bitmask = 1;
                    unsigned int mask = 0;

                    for(int i = 0; i < 33; i++) {
                        bitmask = bitmask << 1u;
                        if(i >= (bytesToSkip - 1)) {
                            mask |= bitmask;
                        }
                    }

                    cout << "Subnet Mask Calculated for prefix: " << address_from_int(&mask) << endl;
                    unsigned masked = mask & mainIp;
                    cout << "Main Network IP calculated from mask: " << address_from_int(&masked) << endl;

                    mainIp = masked;

                    currentMessage = "\nA) Add new subnet\nB) Print subnet calculations\nC) Save calculations to file\nD) Add Multiple Subnets\nR) Reset\n\nCommand> ";
                    step++;
                    continue;
                } else {
                    cout << "Invalid Network IP, make sure to include the prefix" << endl;
                    continue;
                }
            }
            case 1: {
                str = to_upper_copy(str);
                if(str == "A") {
                    currentMessage = "\rNumber of hosts: ";
                    step = 2;
                    continue;
                }
                if(str == "D") {
                    currentMessage = "\r\nType E to exit multiple mode\nNumber of hosts: ";
                    multiple = true;
                    step = 2;
                    continue;
                }
                if(str == "B" || str == "C") {

                    ofstream file;
                    if(str == "C") {
                        file.open("subnet.txt");
                        cout << "Saving to file \'" << "subnet.txt" << "\'" << endl;
                    }

                    (str == "B" ? cout : file) << "\r\nLABEL\t\tADDRESS\t\t\tSUBNET MASK\t\tMAX HOSTS\tUSABLE RANGE\t\t\t\tBROADCAST\t\tPREFIX" << endl;

                    unsigned int initIp = mainIp;

                    sort(subnets.begin(), subnets.end(), [](const Subnet& a, const Subnet& b) -> bool {
                        return a.nextHop > b.nextHop;
                    });

                    for(auto& subnet : subnets) {
                        unsigned int initRange = initIp + 1;
                        unsigned int endRange = initIp + (subnet.nextHop - 2);
                        unsigned int broadcast = initIp + (subnet.nextHop - 1);
                        (str == "B" ? cout : file) << subnet.label << "\t"
                        << address_from_int(&initIp) << "\t\t"
                        << address_from_int(&subnet.s_mask) << "\t\t"
                        << subnet.nextHop - 2 << "\t\t" <<
                        address_from_int(&initRange) << " - " << address_from_int(&endRange) << "\t\t"
                        << address_from_int(&broadcast) << "\t\t"
                        << "/" + to_string(subnet.prefix) <<
                        endl << endl;
                        initIp += subnet.nextHop;
                    }

                    if(file.is_open()) {
                        file.close();
                    }

                    continue;
                }

                if(str == "R") {
                    subnets.clear();
                    cout << "Resetting..." << "\x1B[2J\x1B[H" << endl << flush;
                    currentMessage = "Main Network IP > ";
                    step = 0;
                    continue;
                }
            }

            case 2: {

                if(str == "E" || str == "e") {
                    multiple = false;
                    currentMessage = "\nA) Add new subnet\nB) Print subnet calculations\nC) Save calculations to file\nD) Add Multiple Subnets\nR) Reset\n\nCommand> ";
                    step = 1;
                    continue;
                }

                Subnet subnet{};


                vector<string> optional;
                split(optional, str, is_any_of("-"));

                int hosts = stoi(optional[0]);
                int amount = optional.size() < 2 ? 1 : stoi(optional[1]);

                if(hosts > maxHosts) {
                    cout << "This IP address class only supports up to " << maxHosts << " hosts per subnet." << endl;
                    continue;
                }




                subnet.nextHop = calculate_next_hop(&hosts);
                unsigned int mask = 0;
                unsigned int init = 1;
                int prefix = 0;

                for(int i = 0; i <= 32; i++) {
                    init <<= 1u;
                    if(init >= subnet.nextHop) {
                        prefix++;
                        mask |= init;
                    }
                }

                subnet.s_mask = mask;
                subnet.prefix = prefix;

                for(int i = 0; i < amount; i++) {
                    subnet.label = optional.size() < 3 ? "N" + to_string(random()) : optional[2];
                    subnets.push_back(subnet);
                }

                if(!multiple) {
                    currentMessage = "\nA) Add new subnet\nB) Print subnet calculations\nC) Save calculations to file\nD) Add Multiple Subnets\nR) Reset\n\nCommand> ";
                    step = 1;
                    continue;
                }
                continue;
            }
            default: {
                cout << "Command not found" << endl;
            }
        }
    }
}

unsigned int calculate_next_hop(const int* hosts) {

    cout << "Calculating next hop for " << *hosts << " hosts..." << endl;

    int hop = 1;

    while((hop - 2) < *hosts) {
        hop <<= 1;
    }

    cout << "Next Hop: " << hop << endl;

    return hop;
}