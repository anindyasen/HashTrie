#include <iostream>
#include "mbit_trie.hpp"
#include "../../trunk/5G/UserPlane/ip.hpp"
#include "../../trunk/5G/UserPlane/singleton.hpp"
using namespace std;

int main() {
    //HashTrie<int> newHashTrie;
    unsigned int key = 0x01020304;
    unsigned int data = 2018;
    uint8_t core = 2;
    global::IPHashTrie::Instance().HashTrieInitialize(core);
    global::IPHashTrie::Instance().HashTrieAddNode(key, &data);
    unsigned int key2 = 0x03020202;
    unsigned int data2 = 2019;
    global::IPHashTrie::Instance().HashTrieAddNode(key2, &data2);
    printf("Data::%d\n",*(global::IPHashTrie::Instance().HashTrieGetNode(key)));
    unsigned int *pdata = NULL;
    global::IPHashTrie::Instance().HashTrieRemoveNode(key,&pdata);
    if (NULL == global::IPHashTrie::Instance().HashTrieGetNode(key)) {
        printf("No data found\n");
    }
    else {
        printf("Data::%d\n",*global::IPHashTrie::Instance().HashTrieGetNode(key));
    }
    if (NULL == global::IPHashTrie::Instance().HashTrieGetNode(key2)) {
        printf("No data found\n");
    }
    else {
        printf("Data::%d\n",*global::IPHashTrie::Instance().HashTrieGetNode(key2));
    }
    //global::IPHashTrie::Instance().HashTrieFlushExtended();
    if (NULL == global::IPHashTrie::Instance().HashTrieGetNode(key2)) {
        printf("No data found\n");
    }
    else {
        printf("Data::%d\n",*global::IPHashTrie::Instance().HashTrieGetNode(key2));
    }
    return 0;
}
