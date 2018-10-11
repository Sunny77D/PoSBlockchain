/*------------------------------------------------------------------*/
/* main.cpp of PoSBlockchain                                        */
/* Author: Son Do                                                   */
/* Description: Proof of Stake blockchain to test different         */
/*              consensus, commented part of code still needs to be */
/*              implemented, notably the merkle tree and mining     */
/*------------------------------------------------------------------*/

#include <iostream>

#include <map>
#include <vector>
#include "rapidjson/document.h"
#include <picosha2.h>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <time.h>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/BatchGetItemRequest.h>
#include <aws/core/utils/StringUtils.h>

#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

class CBlock;


    class CBlock {

    public:
        // header:
        int nVersion;

        std::string hash;
        std::string hashPrevBlock;
        Aws::String blockValidator;
        std::string timeStamp;

        // transaction tree
        int hashMerkleRoot;

        unsigned int nUsers;

        unsigned int BPM;
        int index;
        unsigned int nBits;


        /*Transactions:
        // network and disk
        vector<CTransaction> vtx;

        // memory only
        mutable vector<uint256> vMerkleTree;
         */

        //Mining:
        //unsigned int coinsLeft;


        CBlock() {
            SetNull();
        }


        void SetNull() {
            nVersion = 1;
            blockValidator = "null";
            hash = "null";
            hashPrevBlock = "null";
            hashMerkleRoot = 0;
            timeStamp = "null";
            index = 0;

            nBits = 0;
            BPM = 0;

            /* Clearing Transaction Merkle Tree
            vtx.clear();
            vMerkleTree.clear();
             */
        }

        bool isNull() const {
            return (nBits == 0);
        }
    };


std::vector<CBlock> blocks;
std::vector<CBlock> tempBlocks;

//Validators and Stake
std::map<Aws::String, size_t>  validators;
std::map<Aws::String, size_t>  validatorsIn;

//Connected IPs to blockchain server
std::map<SOCKET, Aws::String> connectedIPs;

CBlock candBlock;
std::string display;
std::mutex mutex;




    std::string calculateHash(std::string s) {
        std::string src_str = s;
        std::string hash_hex_string;
        picosha2::hash256_hex_string(src_str, hash_hex_string);
        return hash_hex_string;
    }

    std::string calculateBlockHash(CBlock block) {
        std::string record = std::to_string(block.index) + block.timeStamp +
                block.hashPrevBlock + std::to_string(block.BPM);

        return calculateHash(record);
    }

    CBlock generateBlock(CBlock oldBlock, int BPM, Aws::String address) {
        CBlock newBlock;

        // Creating the new block

        auto time = std::time(nullptr);
        auto tm = *std::localtime(&time);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        auto timeStr = oss.str();
        std::cout << "TimeStamp in block creation: " << timeStr << std::endl;

        newBlock.timeStamp = timeStr;
        newBlock.BPM = BPM;
        newBlock.nVersion = 1;
        // Still need to determine how to get number of users, kioks amount as place holder
        newBlock.index = oldBlock.index + 1;
        newBlock.hashPrevBlock = oldBlock.hash;
        newBlock.hash = calculateBlockHash(newBlock);
        newBlock.blockValidator = address;

        return newBlock;
    }

    bool isBlockValid(CBlock oldBlock, CBlock newBlock) {
        if (oldBlock.index + 1 != newBlock.index) {
            std::cout << "Index is wrong" << std::endl;
            return false;
        }

        if (oldBlock.hash != newBlock.hashPrevBlock) {
            std::cout << "Previous Hash doesn't add up are valid" << std::endl;
            return false;
        }

        if (calculateBlockHash(newBlock) != newBlock.hash) {
            std::cout << "Calculated Hash Doesn't add up with block Hash" << std::endl;
            return false;
        }
        return true;

    }

    // Function to make sure that Validator can propose a block because it hasn't propose one yet
    bool canProposed(std::vector<CBlock> candBlocks, Aws::String validator) {
        if (candBlocks.size() == 0) {
            return true;
        }
        for (size_t i = 0; i < candBlocks.size(); i++) {
            if (validator.compare(candBlocks[i].blockValidator) == 0) {
                std::cout << "VALID IP ADDRESS " << std::endl;
                return false;
            }
        }
        return true;
    }

    // Function to make sure that IP connection is valid
    bool checkIP(std::map<Aws::String, size_t> knownVal, Aws::String connectedIP) {
        if (knownVal.count(connectedIP) == 1) {
            return true;
        }
        return false;
    }

    // Insert a new validator to blockchain
    void insertValidator(Aws::String ipAddy, size_t stake) {
        validators.insert(std::pair<Aws::String, size_t>(ipAddy, stake));
    }

    // Proof of Stake function
    void pickWinner(std::vector<CBlock> &candBlocks) {
        std::chrono::seconds timeSpan(1);
        std::this_thread::sleep_for(timeSpan);
        if (!candBlocks.empty()) {
            std::cout << "Not empty can proceed " << std::endl;
        } else {
            std::cout << "Temp Block is empty can't pick winner "  << std::endl;
            return;
        }

        // Need to implement algorithm to determine winner. Using simple randomizer for now.
        // Every validator has a number which all has same chance of getting picked
        int randNum = rand()%(validatorsIn.size()) + 1;
        Aws::String winnerIP;
        int probFactor;
        for (std::map<Aws::String, size_t>::iterator iter = validatorsIn.begin(); iter != validatorsIn.end(); ++iter) {
            probFactor = iter -> second;
            if (randNum == probFactor) {
                winnerIP = iter->first;
            }
        }
        CBlock pubBlock;
        for (size_t i = 0; i < candBlocks.size(); i++) {
            if(winnerIP.compare(candBlocks[i].blockValidator) == 0) {
                pubBlock = candBlocks[i];
            }
        }
        // Only one block (place holder)
        if (candBlocks.size() == 1) {
            pubBlock = candBlocks[0];
            winnerIP = candBlocks[0].blockValidator;
        }

        // Need to have wallet and to give coins to winner... Can make a map that stores balance easily...
        std::cout << "Winner: " << winnerIP << std::endl;
        blocks.push_back(pubBlock);

        // Testing of new Block on Blockchain
        std::cout << "Size of Block: " <<  blocks.size() << std::endl;
        std::cout << "Time of Block creation: " << blocks[blocks.size()-1].timeStamp << std::endl;

        // Clearing tempBlocks (resetting of Proof of Stake)
        candBlocks.clear();
    }
     void handleConn(fd_set &master, SOCKET listening) {

        // Create a set of sockets
        fd_set copy = master;

        // Number of sockets connected
        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        // Only checks for the socket connections. Does not check for receive messages since routers shouldn't send anything
        for (int i = 0; i < socketCount; i++) {
            SOCKET sock = copy.fd_array[i];
            if (sock == listening) {
                sockaddr_in client;
                int clientSize = sizeof(client);
                SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "invalid socket" << std::endl;
                }

                char host[NI_MAXHOST];
                memset(host, 0, NI_MAXHOST);

                inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
                Aws::String connectedIP = host;
                // Can edit checkIP to encompass more things
                if (checkIP(validators, connectedIP)) {
                    // Insert the validators into PoS algorithm once they're in
                    size_t probFactor = validators[connectedIP];
                    validatorsIn.insert(std::pair<Aws::String, int> (connectedIP, probFactor));

                    FD_SET(clientSocket, &master);
                    connectedIPs.insert(std::pair<SOCKET, Aws::String> (clientSocket, connectedIP));
                    std::cout << connectedIP << " is connected." << std::endl;

                } else {
                    std::cout << "INVALID IP ADDRESS";
                    closesocket(clientSocket);
                }
            }
            else {
                char buf[4096];
                ZeroMemory(buf, 4096);

                int bytesIn = recv(sock, buf, 4096, 0);

                // Client Disconnected
                if (bytesIn <= 0) {
                    // Drop the client
                    std::cout << "IP disconnected: " << connectedIPs[sock];
                    validators.erase(connectedIPs[sock]);
                    connectedIPs.erase(sock);
                    closesocket(sock);
                    FD_CLR(sock, &master);
                } else {
                    int userBPM = 60;

                    std::cout << std::endl;
                    // Make function where you create block only when there the validator hasn't made block yet...
                    if (canProposed(tempBlocks, connectedIPs[sock])) {
                        mutex.lock();
                        CBlock curNewestBlock = blocks[blocks.size() - 1];
                        mutex.unlock();
                        candBlock = generateBlock(curNewestBlock, userBPM, connectedIPs[sock]);

                        if (isBlockValid(curNewestBlock, candBlock)) {
                            tempBlocks.push_back(candBlock);
                            std::cout << "TEMP BLOCK SIZE: " << tempBlocks.size() << std::endl;
                        }
                        else {
                            std::cout << "Block is not valid" << std::endl;
                        }
                    }
                    else {
                        std::cout << "Router can't propose because it already has" << std::endl;
                    }
                }
            }
        }
     }

    int main() {
        // Creating validators
        Aws::SDKOptions options;
        Aws::InitAPI(options);
        {
            // Credentials for aws
            Aws::Auth::AWSCredentials credentials;

            // Empty accessKey and secretKey for Privacy Reasons
            credentials.SetAWSAccessKeyId("");
            credentials.SetAWSSecretKey("");

            Aws::Client::RetryStrategy *defaultRetryStrategy;
            defaultRetryStrategy = new Aws::Client::DefaultRetryStrategy();
            std::shared_ptr<Aws::Client::RetryStrategy> retryStrategy(defaultRetryStrategy);

            Aws::Client::ClientConfiguration configuration;

            configuration.scheme = Aws::Http::Scheme::HTTPS;
            configuration.connectTimeoutMs = 30000;
            configuration.requestTimeoutMs = 30000;
            configuration.retryStrategy = retryStrategy;
            configuration.region = "us-east-1";

            Aws::DynamoDB::DynamoDBClient *dbClient = new Aws::DynamoDB::DynamoDBClient(credentials, configuration);

            // Getting data from database
            Aws::DynamoDB::Model::BatchGetItemRequest req;

            Aws::String Table = "BlockchainTest";
            Aws::DynamoDB::Model::KeysAndAttributes tableKeyAttr;
            tableKeyAttr.SetProjectionExpression("#n, #v");

            Aws::Http::HeaderValueCollection hvc;
            hvc.emplace("#n", "IP address");
            hvc.emplace("#v", "BWPerDay");
            tableKeyAttr.SetExpressionAttributeNames(hvc);

            Aws::DynamoDB::Model::AttributeValue t1key1;

            // Iterating to list of all known Kiosks to get key of info in DB
            for (std::map<Aws::String, size_t>::iterator it = validators.begin(); it != validators.end(); it++) {
                // Grab the info and add it into another vector for the new block (
                Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> t1KeysA;
                t1key1.SetS(it-> first);
                t1KeysA.emplace("ValidatorsIP", t1key1);
                tableKeyAttr.AddKeys(t1KeysA);
            }

            req.AddRequestItems("BlockchainData", tableKeyAttr);


            // Getting Blockchain data from the database base
            const Aws::DynamoDB::Model::BatchGetItemOutcome &result = dbClient->BatchGetItem(req);
            // Would store them in variable instead of print them.
            if (result.IsSuccess()) {
                for (const auto &var : result.GetResult().GetResponses()) {
                    Aws::String tableName = var.first;
                    // Printing out table to make sure data is beginning sent correctly
                    std::cout << tableName << std::endl;
                    std::cout << "IP address | Stake " << std::endl;
                    for (const auto &itm : var.second) {

                        Aws::String validator = itm.at("ValidatorsIP").GetS();
                        Aws::String stake = itm.at("Stake").GetN();
                        size_t stakeN = atoi(stake.c_str());
                        validators.insert(std::pair<Aws::String, size_t>(validator, stakeN));
                        std::cout << std::endl;
                    }
                }
            } else {
                std::cout << "Failed to get item";
            }
        }
        Aws::ShutdownAPI(options);

        CBlock org;
        org.hash = "n/a";
        org.BPM = 2;
        org.index = 1;
        org.hashPrevBlock = "n/a";
        org.nBits = 4;
        org.nVersion = 1;
        org.hashPrevBlock ="0000";
        org.hash = calculateBlockHash(org);
        org.index = 0;

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        auto str = oss.str();
        org.timeStamp = str;

        blocks.push_back(org);

        CBlock nBlock = generateBlock(org, 4, "Local");
        blocks.push_back(nBlock);
        // Testing info in new Block

        std::cout << "Checking Block Valid Method: " << isBlockValid(org, nBlock) << std::endl;

        std::cout << "BlockValidator: " << nBlock.blockValidator << std::endl;
        std::cout << "TimeStamp: " << nBlock.timeStamp << std::endl;
        std::cout << "Hash: " << nBlock.hash << std::endl;
        std::cout << "OldHash: " << org.hash << std::endl;
        std::cout << "NewBlock Previous Hash: " << nBlock.hashPrevBlock << std::endl;

        // Setting up Server
        WSADATA wsdata;
        WORD ver = MAKEWORD(2, 2);
        // Initialize winsocket
        int wsOk = WSAStartup(ver, &wsdata);
        if (wsOk != 0) {
            std::cerr << "Can't Initialize winsock! Quitting" << std::endl;
            return 0;
        }

        SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
        if (listening == INVALID_SOCKET) {
            std::cerr << "Can't create a socket! Quitting" << std::endl;
            return 0;
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(54000);
        hint.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(listening, (sockaddr*)&hint, sizeof(hint));

        listen(listening, SOMAXCONN);

        fd_set master;
        FD_ZERO (&master);

        FD_SET(listening, &master);

        // Testing PoS:

        bool running = true;

        // Listening for connections
        while (running) {

            // Handle the connection and creating temp blocks
            handleConn(master, listening);
            //Create function to only pick winner once people actually connected or there's temporary blocks!
            // Doesn't pick winner automatically, gotta wait to handleConn does something hahaha...
            pickWinner(tempBlocks);
            // Delete the temp blocks in pickWinner. Double check a block has been proposed by every kiosks
        }
    }
