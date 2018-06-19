// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_MBIT_TRIE_HPP_
#define USERPLANE_MBIT_TRIE_HPP_

#include <utility>
#include <memory>
/**
 * Multi Bit Trie Arch. Used for IPv4 address lookup
 *
 */
#include "common.hpp"
#include "lock_rcu.hpp"
#include "heap_manager.hpp"
#include "logger.hpp"
#include "ip.hpp"

namespace hash {
const int kHashTrieSize = 256;

template <typename T>
struct NodesD {
    T*           dataPtr[kHashTrieSize];
    uint16_t     EffectiveNodeCount;
};
template <typename T>
struct NodesC {
    NodesD<T>* TierNode[kHashTrieSize];
    uint16_t   EffectiveNodeCount;
};

template <typename T>
struct NodesB {
    NodesC<T>* TierNode[kHashTrieSize];
    uint16_t  EffectiveNodeCount;
};

template <typename T>
class HashTrie {
 public:
    virtual ~HashTrie() {
        HashTrieFlushExtended();
    }
    utils::RESULT       HashTrieInitialize(uint8_t coreId = 0);
    utils::RESULT       HashTrieAddNode(uint32_t in_Key, T *in_Data);
    bool                HashTrieRemoveNode(uint32_t in_Key, T** result);
    T*                  HashTrieGetNode(uint32_t in_Key);

 private:
    lock::RCUProtected<NodesB<T>> BaseNodesPtrArr_[kHashTrieSize];
    uint16_t                      EffectiveNodeCount_;
    uint8_t                       WorkCore_;

    uint8_t       GetTrieKey(uint32_t in_Key, int in_pos);
    uint32_t      Accumulated_Key(int in_key1, int in_key2, int in_key3, int in_key4);
    NodesB<T>*    GetReadNextNode(int idx);
    void          FinalizeReadingNextNode(int idx);
    NodesB<T>*    GetWriteNextNode(int idx);
    void          SyncBeforeUpdateNextNode(int idx);
    NodesB<T>*    UpdateNextNode(NodesB<T>*, int idx);
    void          HashTrieFlushExtended();
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
NodesB<T>* HashTrie<T>::UpdateNextNode(NodesB<T>* newNextNode, int idx) {
    return BaseNodesPtrArr_[idx].update(newNextNode);
}

template <typename T>
void HashTrie<T>::SyncBeforeUpdateNextNode(int idx) {
    BaseNodesPtrArr_[idx].synchronize_writing();
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
    return (kHashTrieSize-1);
}

template <typename T>
always_inline
uint32_t HashTrie<T>::Accumulated_Key(int in_key1, int in_key2, int in_key3, int in_key4) {
    return (((in_key1<<24) & 0xFF) | ((in_key2<<16) & 0xFF) |
             ((in_key3<<8) & 0xFF) | ((in_key4) & 0xFF));
}

template <typename T>
utils::RESULT HashTrie<T>::HashTrieInitialize(uint8_t coreId) {
    WorkCore_ = coreId;
    for (int i = 0 ; i < kHashTrieSize ; i++) {
        BaseNodesPtrArr_[i].setCoreId(coreId);
    }
    return utils::RESULT::OK;
}

template <typename T>
utils::RESULT HashTrie<T>::HashTrieAddNode(uint32_t in_Key, T *in_Data) {
    uint8_t Tier1Key = GetTrieKey(in_Key, 1);
    uint8_t Tier2Key = GetTrieKey(in_Key, 2);
    uint8_t Tier3Key = GetTrieKey(in_Key, 3);
    uint8_t Tier4Key = GetTrieKey(in_Key, 4);

    if ((kHashTrieSize-1) == Tier1Key || (kHashTrieSize-1) == Tier2Key ||
        (kHashTrieSize-1) == Tier3Key || (kHashTrieSize-1) == Tier4Key) {
        return utils::RESULT::ERROR;
    }

    NodesB<T> *TierNode1 = GetReadNextNode(Tier1Key);
    if (nullptr == TierNode1) {
        // allocating memory only
        NodesB<T> *NewTierNode =
        reinterpret_cast<NodesB<T> *>(heap::MiniMalloc(WorkCore_, sizeof(NodesB<T>)));
        // calling constructor
        new (NewTierNode) NodesB<T>();
        EffectiveNodeCount_++;
        FinalizeReadingNextNode(Tier1Key);
        UpdateNextNode(NewTierNode, Tier1Key);  // called update and not sync_update
        TierNode1 = GetReadNextNode(Tier1Key);
    }

    if (nullptr == TierNode1->TierNode[Tier2Key]) {
        NodesC<T>* NewNodesCPtr =
                      reinterpret_cast<NodesC<T> *>(heap::MiniMalloc(WorkCore_, sizeof(NodesC<T>)));
        new (NewNodesCPtr) NodesC<T>();
        TierNode1->TierNode[Tier2Key] = NewNodesCPtr;
        TierNode1->EffectiveNodeCount++;
    }

    NodesC<T> *TierNode2 = TierNode1->TierNode[Tier2Key];
    if (nullptr == TierNode2->TierNode[Tier3Key]) {
        NodesD<T>* NewNodeDPtr =
                      reinterpret_cast<NodesD<T> *>(heap::MiniMalloc(WorkCore_, sizeof(NodesD<T>)));
        new (NewNodeDPtr) NodesD<T>();
        TierNode2->TierNode[Tier3Key] = NewNodeDPtr;
        TierNode2->EffectiveNodeCount++;
    }

    NodesD<T> *TierNode3 = TierNode2->TierNode[Tier3Key];
    if (nullptr == TierNode3->dataPtr[Tier4Key]) {
        TierNode3->dataPtr[Tier4Key] = in_Data;
        TierNode3->EffectiveNodeCount++;
        FinalizeReadingNextNode(Tier1Key);
        return utils::RESULT::OK;
    }
    FinalizeReadingNextNode(Tier1Key);
    return utils::RESULT::ERROR;
}

template <typename T>
T* HashTrie<T>::HashTrieGetNode(uint32_t in_Key) {
    uint8_t Tier1Key = GetTrieKey(in_Key, 1);
    uint8_t Tier2Key = GetTrieKey(in_Key, 2);
    uint8_t Tier3Key = GetTrieKey(in_Key, 3);
    uint8_t Tier4Key = GetTrieKey(in_Key, 4);

    T *ret = NULL;

    if ((kHashTrieSize-1) == Tier1Key || (kHashTrieSize-1) == Tier2Key ||
        (kHashTrieSize-1) == Tier3Key || (kHashTrieSize-1) == Tier4Key) {
        return ret;
    }

    NodesB<T> *Tire2 = nullptr;
    NodesC<T> *Tire3 = nullptr;
    NodesD<T> *Tire4 = nullptr;


    Tire2 = GetReadNextNode(Tier1Key);

    if (nullptr != Tire2) {
        Tire3 = Tire2->TierNode[Tier2Key];
        if (nullptr != Tire3) {
            Tire4 = Tire3->TierNode[Tier3Key];
            if (nullptr != Tire4) {
                ret = Tire4->dataPtr[Tier4Key];
            }
        }
    }
    FinalizeReadingNextNode(Tier1Key);
    return ret;
}

template <typename T>
bool HashTrie<T>::HashTrieRemoveNode(uint32_t in_Key, T** result) {
    if (result == nullptr) {
        logger::LOG("Failed to remove key from Hash table.\n");
        return false;
    }

    uint8_t Tier1Key = GetTrieKey(in_Key, 1);
    uint8_t Tier2Key = GetTrieKey(in_Key, 2);
    uint8_t Tier3Key = GetTrieKey(in_Key, 3);
    uint8_t Tier4Key = GetTrieKey(in_Key, 4);

    if ((kHashTrieSize-1) == Tier1Key || (kHashTrieSize-1) == Tier2Key ||
        (kHashTrieSize-1) == Tier3Key || (kHashTrieSize-1) == Tier4Key) {
        return false;
    }

    T* pData = nullptr;

    NodesB<T> *Tire2 = GetReadNextNode(Tier1Key);

    if (nullptr != Tire2) {
        NodesC<T> *Tire3 = Tire2->TierNode[Tier2Key];
        if (nullptr != Tire3) {
            NodesD<T> *Tire4 = Tire3->TierNode[Tier3Key];
            if (nullptr != Tire4) {
                pData = Tire4->dataPtr[Tier4Key];
            }
        }
    }
    FinalizeReadingNextNode(Tier1Key);


    if (nullptr != pData) {
        NodesB<T> *Tire2 = GetWriteNextNode(Tier1Key);
        NodesC<T> *Tire3 = Tire2->TierNode[Tier2Key];
        NodesD<T> *Tire4 = Tire3->TierNode[Tier3Key];

        *result = Tire4->dataPtr[Tier4Key];
        Tire4->EffectiveNodeCount--;
        if (0 == Tire4->EffectiveNodeCount) {
            Tire3->TierNode[Tier3Key] = nullptr;
            Tire3->EffectiveNodeCount--;
        } else {
            Tire4 = nullptr;  //  No need to delete Tire4
        }

        if (0 == Tire3->EffectiveNodeCount) {
            Tire2->TierNode[Tier2Key] = nullptr;
            Tire2->EffectiveNodeCount--;
        } else {
            Tire3 = nullptr;  //  No need to delete Tire3
        }
        NodesB<T> *OldTire2 = nullptr;

        if (0 == Tire2->EffectiveNodeCount) {
            EffectiveNodeCount_--;
            OldTire2 = UpdateNextNode(nullptr, Tier1Key);
        }

        SyncBeforeUpdateNextNode(Tier1Key);

        if (nullptr != OldTire2) {
            if (nullptr != OldTire2) {
                heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&OldTire2));
            }
        }

        if (nullptr != Tire4) {
           heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&Tire4));
        }

        if (nullptr != Tire3) {
           heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&Tire3));
        }

        return true;
    }
    return false;
}

template <typename T>
void HashTrie<T>::HashTrieFlushExtended() {
    for (int i = 0; i < kHashTrieSize; i++) {
        NodesB<T> *Tire2 = GetWriteNextNode(i);
        if (nullptr != Tire2) {
            for (int j = 0; j < kHashTrieSize; j++) {
                if (nullptr != Tire2->TierNode[j]) {
                    NodesC<T> *Tire3 = Tire2->TierNode[j];
                    for (int k = 0; k < kHashTrieSize; k++) {
                        if (nullptr != Tire3->TierNode[k]) {
                            NodesD<T> *Tire4 = Tire3->TierNode[k];
                            for (k = 0; k < kHashTrieSize; k++) {
                                Tire4->dataPtr[k] = nullptr;
                            }
                            Tire4->EffectiveNodeCount = 0;
                            SyncBeforeUpdateNextNode(i);
                            heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&Tire4));
                        }
                        Tire3->TierNode[k] = nullptr;
                    }
                    Tire3->EffectiveNodeCount = 0;
                    SyncBeforeUpdateNextNode(i);
                    heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&Tire3));
                }
                Tire2->TierNode[j] = nullptr;
            }
            Tire2->EffectiveNodeCount = 0;
            NodesB<T> * oldTire2 = UpdateNextNode(nullptr, i);
            SyncBeforeUpdateNextNode(i);
            heap::MiniFree(WorkCore_, reinterpret_cast<void**>(&oldTire2));
        }
    }
    EffectiveNodeCount_ = 0;
}
}  //  namespace hash


//Usage :
//Create Singleton Object(Here ip::IPv4Addr_t is the type of data in trie):
namespace global {
using IPHashTrie = Singleton<hash::HashTrie<ip::IPv4Addr_t>>;
}  //  namespace global

//Initialize IPHashTrie :
//global::IPHashTrie::Instance().HashTrieInitialize(core);

#endif  // USERPLANE_MBIT_TRIE_HPP_
