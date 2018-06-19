// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_UTILS_MBIT_TRIE_HPP_
#define USERPLANE_UTILS_MBIT_TRIE_HPP_

/**
 * Multi Bit Trie Arch. Used for IPv4 address lookup
 * 
 */
#include "utils_rcu.hpp"

const int HASH_TRIE_SIZE = 256;

template <typename T>
struct UserData {
    T *data;
};

template <typename T>
struct NodesD {
    UserData<T>* TierNode[HASH_TRIE_SIZE];
    uint16_t  EffectiveNodeCount;
};

template <typename T>
struct NodesC {
    NodesD<T>* TierNode[HASH_TRIE_SIZE];
    uint16_t  EffectiveNodeCount;
};

template <typename T>
struct NodesB {
    NodesC<T>* TierNode[HASH_TRIE_SIZE];
    uint16_t  EffectiveNodeCount;
};


template <typename T>
class HashTrie {
 public:
    NodesB<T>*    GetReadNextNode(int idx);
    void          FinalizeReadingNextNode(int idx);
    NodesB<T>*    GetWriteNextNode(int idx);
    void          UpdateNextNode(NodesB<T>*, int idx);
    HashTrie*     HashTrieInitialize();
    void          HashTrieFinalize();
    void          HashTrieFlushExtended();
    int           HashTrieAddNode(uint32_t in_uiKey, T *in_pvData);
    int           HashTrieRemoveNode(uint32_t in_uiKey, T **out_ppvData);
    T*            HashTrieGetNode(uint32_t in_uiKey);
 private:
    lock::RCUProtected<NodesB<T>> BaseNodesPtrArr_[HASH_TRIE_SIZE];
    uint16_t              EffectiveNodeCount_;
    always_inline
    uint8_t        GetTrieKey(uint32_t in_Key, int in_pos);
    always_inline
    uint32_t       Accumulated_Key(int in_key1, int in_key2, int in_key3, int in_key4);
};

template <typename T>
NodesB<T>* HashTrie<T>::GetReadNextNode(int idx) {
    return BaseNodesPtrArr_[idx].get_reading_copy_protected();
}

template <typename T>
void HashTrie<T>::FinalizeReadingNextNode(int idx) {
    BaseNodesPtrArr_[idx].finalize_reading();
}

template <typename T>
NodesB<T>* HashTrie<T>::GetWriteNextNode(int idx) {
    return BaseNodesPtrArr_[idx].get_updating_copy();
}

template <typename T>
void HashTrie<T>::UpdateNextNode(NodesB<T>* newNextNode, int idx) {
    BaseNodesPtrArr_[idx].update(newNextNode);
}

template <typename T>
HashTrie<T> * HashTrie<T>::HashTrieInitialize() {
    HashTrie *psHashTierBase = new HashTrie;
    return psHashTierBase;
}

template <typename T>
void HashTrie<T>::HashTrieFinalize() {
    HashTrieFlushExtended();
}

template <typename T>
always_inline
uint8_t HashTrie<T>::GetTrieKey(uint32_t in_Key, int in_pos) {
    switch (in_pos) {
        case 1:
            return ((in_Key >> 24) & 0xFF);
        case 2:
            return ((in_Key >> 16) & 0xFF);
        case 3:
            return ((in_Key >> 8) & 0xFF);
        case 4:
            return ((in_Key) & 0xFF);
    }
    return utils::ERROR;
}

template <typename T>
always_inline
uint32_t HashTrie<T>::Accumulated_Key(int in_key1, int in_key2, int in_key3, int in_key4) {
    return (((in_key1<<24) & 0xFF) | ((in_key2<<16) & 0xFF) |
             ((in_key3<<8) & 0xFF) | ((in_key4) & 0xFF));
}


template <typename T>
int HashTrie<T>::HashTrieAddNode(uint32_t in_uiKey, T *in_pvData) {
    uint8_t ucTier1Key = GetTrieKey(in_uiKey, 1);
    uint8_t ucTier2Key = GetTrieKey(in_uiKey, 2);
    uint8_t ucTier3Key = GetTrieKey(in_uiKey, 3);
    uint8_t ucTier4Key = GetTrieKey(in_uiKey, 4);

    NodesB<T> *TierNode1 = GetReadNextNode(ucTier1Key);
    if (NULL == TierNode1) {
        NodesB<T> *NewTierNode = new NodesB<T>;
        EffectiveNodeCount_++;
        FinalizeReadingNextNode(ucTier1Key);
        UpdateNextNode(NewTierNode, ucTier1Key);
        TierNode1 = GetReadNextNode(ucTier1Key);
    }

    if (NULL == TierNode1->TierNode[ucTier2Key]) {
       TierNode1->TierNode[ucTier2Key] = new NodesC<T>;
       TierNode1->EffectiveNodeCount++;
    }

    NodesC<T> *TierNode2 = TierNode1->TierNode[ucTier2Key];
    if (NULL == TierNode2->TierNode[ucTier3Key]) {
       TierNode2->TierNode[ucTier3Key] = new NodesD<T>;
       TierNode2->EffectiveNodeCount++;
    }

    NodesD<T> *TierNode3 = TierNode2->TierNode[ucTier3Key];
    if (NULL == TierNode3->TierNode[ucTier4Key]) {
       UserData<T> *in_data = new UserData<T>;
       in_data->data = in_pvData;
       TierNode3->TierNode[ucTier4Key] = in_data;
       TierNode3->EffectiveNodeCount++;
       FinalizeReadingNextNode(ucTier1Key);
       return utils::OK;
    }
    FinalizeReadingNextNode(ucTier1Key);
    return utils::ERROR;
}

template <typename T>
T*  HashTrie<T>::HashTrieGetNode(uint32_t in_uiKey) {
    uint8_t ucTier1Key = GetTrieKey(in_uiKey, 1);
    uint8_t ucTier2Key = GetTrieKey(in_uiKey, 2);
    uint8_t ucTier3Key = GetTrieKey(in_uiKey, 3);
    uint8_t ucTier4Key = GetTrieKey(in_uiKey, 4);

    NodesB<T> *psTire2 = NULL;
    NodesC<T> *psTire3 = NULL;
    NodesD<T> *psTire4 = NULL;

    psTire2 = GetReadNextNode(ucTier1Key);

    if (NULL != psTire2) {
        psTire3 = psTire2->TierNode[ucTier2Key];
        if (NULL != psTire3) {
            psTire4 = psTire3->TierNode[ucTier3Key];
            if (NULL != psTire4) {
                FinalizeReadingNextNode(ucTier1Key);
                return psTire4->TierNode[ucTier4Key]->data;
            }
        }
    }
    FinalizeReadingNextNode(ucTier1Key);
    return NULL;
}

template <typename T>
int HashTrie<T>::HashTrieRemoveNode(uint32_t in_uiKey, T **out_ppvData) {
    uint8_t ucTier1Key = GetTrieKey(in_uiKey, 1);
    uint8_t ucTier2Key = GetTrieKey(in_uiKey, 2);
    uint8_t ucTier3Key = GetTrieKey(in_uiKey, 3);
    uint8_t ucTier4Key = GetTrieKey(in_uiKey, 4);

    NodesB<T> *psTire2 = NULL;
    NodesC<T> *psTire3 = NULL;
    NodesD<T> *psTire4 = NULL;

    T *pdata = NULL;

    psTire2 = GetReadNextNode(ucTier1Key);

    if (NULL != psTire2) {
        psTire3 = psTire2->TierNode[ucTier2Key];
        if (NULL != psTire3) {
            psTire4 = psTire3->TierNode[ucTier3Key];
            if (NULL != psTire4) {
                pdata = psTire4->TierNode[ucTier4Key]->data;
            }
        }
    }
    FinalizeReadingNextNode(ucTier1Key);
    if (NULL != out_ppvData) {
        *out_ppvData = pdata;
    }

    if (NULL != pdata) {
        psTire4->TierNode[ucTier4Key] = NULL;
        psTire4->EffectiveNodeCount--;
        if (0 == psTire4->EffectiveNodeCount) {
            psTire3->TierNode[ucTier3Key] = NULL;
            psTire3->EffectiveNodeCount--;
            delete psTire4;
        }

        if (0 == psTire3->EffectiveNodeCount) {
            psTire2->TierNode[ucTier2Key] = NULL;
            psTire2->EffectiveNodeCount--;
            delete psTire3;
        }

        if (0 == psTire2->EffectiveNodeCount) {
            EffectiveNodeCount_--;
            UpdateNextNode(NULL,ucTier1Key);
        }
        return utils::OK;
    }
    return utils::ERROR;
}


template <typename T>
void HashTrie<T>::HashTrieFlushExtended() {
    int i, j, k;
    for (i = 0; i < HASH_TRIE_SIZE; i++) {
        NodesB<T> *Tire2 = GetWriteNextNode(i);
        if (NULL != Tire2) {
            for (j = 0; j < HASH_TRIE_SIZE; j++) {
                if (NULL != Tire2->TierNode[j]) {
                    NodesC<T> *Tire3 = Tire2->TierNode[j];
                    for (k = 0; k < HASH_TRIE_SIZE; k++) {
                        if (NULL != Tire3->TierNode[k]) {
                            NodesD<T> *Tire4 = Tire3->TierNode[k];
                            for (k = 0; k < HASH_TRIE_SIZE; k++) {
                                Tire4->TierNode[k] = NULL;
                            }
                            Tire4->EffectiveNodeCount = 0;
                            delete Tire4;
                        }
                        Tire3->TierNode[k] = NULL;
                    }
                    Tire3->EffectiveNodeCount = 0;
                    delete Tire3;
                }
                Tire2->TierNode[j] = NULL;
            }
            Tire2->EffectiveNodeCount = 0;
            UpdateNextNode(NULL,i);
        }
    }
    EffectiveNodeCount_ = 0;
}

#endif  // USERPLANE_UTILS_MBIT_TRIE_HPP_
