//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// groupd 6
// stage 4
// group member: 
// Student Name:jack chen, 
// Email: jchen488@wisc.edu
// Student Name:conard chen, 
// Email: wchen283@wisc.edu
// Student Name:yudong huang
// Email: huang293@wisc.edu
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "btree.h"
#include "filescan.h"
#include "file.h"
#include <stdlib.h>
#include <stdio.h>
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/end_of_file_exception.h"

bool debug = false;
//#define DEBUG

namespace badgerdb
{
    // -----------------------------------------------------------------------------
    // Some utility functions
    // -----------------------------------------------------------------------------
    
    bool BTreeIndex::is_full(NonLeafNodeInt *node){
        return node->size == INTARRAYNONLEAFSIZE - 1;
    }

    bool BTreeIndex::is_full(LeafNodeInt *node){
        return node->size == INTARRAYLEAFSIZE - 1;
    }

    int BTreeIndex::b_search(int *arr, int size, int key){
        int low_bound = 0;
        // a special case, when size == 2, i == 1, j == 2
        // where j overflows the array
        if (size == 2){
            low_bound = -1;
        }
        int up_bound = size;
        int i = 0;
        int j = 0;
        while (1) {
            i = (low_bound + up_bound) / 2;
            j = i + 1;
            if (arr[i] <= key && arr[j] > key) {
                return i;
            }
            if (arr[i] > key) {
                up_bound = i;
            }   
            if (arr[j] <= key) {
                low_bound = i;
            }
        } 
    }

    PageId BTreeIndex::kb_search(NonLeafNodeInt *node, int key){
        if (node->size == 0){
            return 0;
        }
        int *arr = node->keyArray; 
        if (arr[0] > key){
            return node->pageNoArray[0];
        }
        if (arr[node->size - 1] <= key){
            return node->pageNoArray[node->size];
        }
        return node->pageNoArray[b_search(arr, node->size, key) + 1];
    }

    int BTreeIndex::kb_search(LeafNodeInt *node, int key){
        // ridArray size == keyArray size 
        int *arr = node->keyArray;
        if (key > arr[node->size - 1]){
            return -2;
        } 
        if (key < arr[0]) {
            return -1;
        }
        // since binary search returns the index where Ki <= key < Ki+1,
        // so the last index need to be covered here, otherwise j will
        // overflow the array.
        if (key == arr[node->size - 1]){
            return node->size - 1;
        }
        return b_search(arr, node->size, key);
    }

    void BTreeIndex::add_key(NonLeafNodeInt* node, int key, PageId pid) {
        int i = 0;
        int *arr = node->keyArray;
        
        //std::cout << "add_key" << std::endl;
        if (arr[0] > key){
            i = 0;
        } else if (arr[node->size - 1] <= key || node->size == 0){
            arr[node->size] = key;
            node->size++;
            node->pageNoArray[node->size] = pid;
            return;
        } else {
            i = b_search(arr, node->size, key) + 1;
        }
        
        node->size++;
        if (node->size > INTARRAYNONLEAFSIZE){
            std::cout << "add_key::node size greater than max" << std::endl;
        }
        for (int j = node->size - 2; j >= i; j--) {
            arr[j+1] = arr[j];
        }
        arr[i] = key;
        PageId *parr = node->pageNoArray;
        for (int j = node->size - 1; j >= i+1; j--) {
            parr[j+1] = parr[j];
        }
        parr[i+1] = pid;
    }

    void BTreeIndex::add_key(LeafNodeInt* curr_node, RIDKeyPair *entry){
        curr_node->keyArray[curr_node->size] = entry->key;
        curr_node->ridArray[curr_node->size] = entry->rid;
        curr_node->size++;   
    }

    void BTreeIndex::insert_key(LeafNodeInt* node, RIDKeyPair *entry) {
        int i = 0;
        int key = entry->key;
        int *arr = node->keyArray;
        for (i = 0; i < node->size; i++) {
            if (key < arr[i]){
                break;
            }    
        }
        node->size++;
        if (node->size > INTARRAYLEAFSIZE){
            std::cout << "add_key::node size greater than max" << std::endl;
        }
        for (int j = node->size - 2; j > i; j--){
            arr[j+1] = arr[j];
        }
        arr[i] = entry->key;
        RecordId *rarr = node->ridArray;
        for (int j = node->size - 2; j > i; j--){
            rarr[j+1] = rarr[j];
        }
        rarr[i] = entry->rid;
    }

    void BTreeIndex::split(NonLeafNodeInt* node, NonLeafNodeInt* node2) {
        int break_point = node->size / 2;
        int *arr1 = node->keyArray;
        int *arr2 = node2->keyArray;
        // split key array
        for (int i = break_point + 1; i < node->size; i++){
            arr2[node2->size] = arr1[i];
            node2->size++;
        }
        PageId *parr1 = node->pageNoArray;
        PageId *parr2 = node2->pageNoArray;
        // split pid array, separate with splitting key array 
        // to achieve better spatial locality
        for (int i = break_point + 1, j = 0; i < node->size + 1; i++, j++){
            parr2[j] = parr1[i];
        }
        if ((node->size - 1) != (node2->size + break_point)) {
            std::cout << "split::miscalculation of key array size" << std::endl;
        }
        node->size = break_point;
        node2->level = node->level;
    }

    void BTreeIndex::split(LeafNodeInt* node, LeafNodeInt* node2, PageId pid2){
        int break_point = node->size / 2;
        //int old_size = 0;
        int *arr1 = node->keyArray;
        int *arr2 = node2->keyArray;
        // split key array
        for (int i = break_point; i < node->size; i++){
            arr2[node2->size] = arr1[i];
            node2->size++;
        }
        RecordId *rarr1 = node->ridArray;
        RecordId *rarr2 = node2->ridArray;
        // split rid array, separate with splitting key array 
        // to achieve better spatial locality
        for (int i = break_point, j = 0; i < node->size; i++, j++){
            rarr2[j] = rarr1[i];
        }
        node->size = break_point;
        PageId rightSib = 0;
        if (node->rightSibPageNo != 0) {
            rightSib = node->rightSibPageNo;
        }
        node->rightSibPageNo = pid2;
        node2->rightSibPageNo = rightSib;
    }

    int BTreeIndex::ran(int left, int right) {
        return left + (rand() % static_cast<int>(right - left + 1));
    }

    void BTreeIndex::qsort(LeafNodeInt* node, int left, int right) {
        int i = left;
        int j = right;
        int tmp_key;
        RecordId tmp_rid;
        int *arr = node->keyArray;
        RecordId *rarr = node->ridArray;
        int pivot = arr[ran(left, right)];
        
        //partition
        while (i <= j){
            while(arr[i] < pivot)
                i++;
            while(arr[j] > pivot)
                j--;
            if (i <= j){
                tmp_key = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp_key;

                tmp_rid = rarr[i];
                rarr[i] = rarr[j];
                rarr[j] = tmp_rid; 

                i++;
                j--;
            }
        }; 
        if (left < j)
            qsort(node, left, j);
        if (i < right)
            qsort(node, i, right);
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::BTreeIndex -- Constructor
    // -----------------------------------------------------------------------------

    BTreeIndex::BTreeIndex(const std::string & relationName,
            std::string & outIndexName,
            BufMgr *bufMgrIn,
            const int attrByteOffset,
            const Datatype attrType)
    {
        this->bufMgr = bufMgrIn;
        this->attributeType = attrType; 
        this->attrByteOffset = attrByteOffset; 
        this->createExecuting = false;
        std::ostringstream indexStr;
        indexStr << relationName << '.' << attrByteOffset;
        std::string indexName = indexStr.str();

        if (File::exists(indexName)){
            openIndexFile(indexName);
        } else {
            createIndexFile(indexName, relationName);
        }
        outIndexName = indexName;
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::~BTreeIndex -- destructor
    // -----------------------------------------------------------------------------

    BTreeIndex::~BTreeIndex()
    {
        Page *metaPage;
        this->bufMgr->readPage(this->file, this->headerPageNum, metaPage);
        IndexMetaInfo *metaInfo = (IndexMetaInfo*)metaPage;
        metaInfo->rootPageNo = this->rootPageNum;
        this->bufMgr->unPinPage(this->file, this->headerPageNum, true);
        this->bufMgr->flushFile(this->file);
        std::cout << "Index File flushed" << std::endl;
        this->scanExecuting = false;
        delete(this->file); 
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::insertEntry
    // -----------------------------------------------------------------------------
    void BTreeIndex::insertEntry(Page* curr_page, RIDKeyPair *entry, PageKeyPair **newEntry, bool is_leaf){ 
        //never unpin curr_page, curr_page should be unpinned by previous stack frame
        PageId next_pid = 0;
        Page *next_page;
        if (!is_leaf) {
            NonLeafNodeInt* curr_node = (NonLeafNodeInt*)curr_page;
            next_pid = kb_search((NonLeafNodeInt*)curr_node, entry->key);     
            if (next_pid == 0){
                //when the current node is empty
                //the first two empty leaf is initialized at createIndexTree()
                curr_node->keyArray[curr_node->size] = entry->key;
                curr_node->size++;
                next_pid = curr_node->pageNoArray[curr_node->size];
            }
            //REMEMBER to unpin next_pid later
            this->bufMgr->readPage(this->file, next_pid, next_page); 
            
            //if current node's level is 0, next node is leaf node
            bool next_is_leaf = false;
            if (curr_node->level == 0){
                //std::cout << "next leaf is true" << std::endl;
                next_is_leaf = true;
            }
            insertEntry(next_page, entry, newEntry, next_is_leaf);
            
            if (*newEntry == NULL){
                this->bufMgr->unPinPage(this->file, next_pid, true);
                return;
            } else {
                if (!is_full(curr_node)) {
                    // insert, nonleafnodes must be sorted, 
                    // otherwise kb_search is not going to work
                    // (the better way is to use hashtable)
                    add_key(curr_node, (*newEntry)->key, (*newEntry)->pageNo);
                    delete(*newEntry);
                    *newEntry = NULL;
                    this->bufMgr->unPinPage(this->file, next_pid, true);
                    return;
                } else {
                    // if current nonleaf page is full
                    // split node
                    // can still add one more since max size is INTARRAYNONLEAFSIZE - 1 
                    add_key(curr_node, (*newEntry)->key, (*newEntry)->pageNo);
                    Page *page2 = NULL;
                    PageId pid2 = 0;
                    newPage(&pid2, page2);
                    NonLeafNodeInt *node2 = (NonLeafNodeInt*)page2;
                    node2->pid = pid2;
                    
                    split(curr_node, node2);

                    *newEntry = new PageKeyPair;
                    (*newEntry)->set(pid2, node2->keyArray[0]);
                    //to distinguish root page, mustn't use root page pointer
                    //because page pointer referes to some address in buffer
                    //pool, which is nondeterministic
                    if (curr_node->pid == rootPageNum) {
                        Page *new_root_page = NULL;
                        newPage(&this->rootPageNum, new_root_page);
                        NonLeafNodeInt* new_root_node = (NonLeafNodeInt*)new_root_page;
                        new_root_node->level = curr_node->level + 1;
                        new_root_node->pid = this->rootPageNum;
                        new_root_node->pageNoArray[0] = curr_node->pid;
                        add_key(new_root_node, (*newEntry)->key, (*newEntry)->pageNo);
                        delete(*newEntry);
                        *newEntry = NULL;
                        this->bufMgr->unPinPage(this->file, this->rootPageNum, true); 
                    }
                    this->bufMgr->unPinPage(this->file, pid2, true);
                    this->bufMgr->unPinPage(this->file, next_pid, true);
                    return;
                }
            }
        } else {
            //if current page is a leaf page
            LeafNodeInt* curr_node = (LeafNodeInt*)curr_page;
            if (!is_full(curr_node)){
                // leaf nodes should mark itself unsorted when a key is inserted
                if (this->createExecuting){
                    add_key(curr_node, entry);
                    curr_node->sorted = false;
                } else {
                    insert_key(curr_node, entry);
                    curr_node->sorted = true;
                }
                *newEntry = NULL;
                return;
            } else {
                //split leaf node, remember to mark new nodes sorted as well
                //first, add entry to node, should be able to add one more 
                //when is_full return true. 
                //second, sort the leaf node, must sort two arrays in parallel
                if (this->createExecuting){
                    add_key(curr_node, entry);
                    qsort(curr_node, 0, curr_node->size - 1);
                } else {
                    //no need to sort during normal insertion
                    insert_key(curr_node, entry);
                }
                //split leaf node into two leaf nodes, and mark both of them sorted
                Page *page2 = NULL;
                PageId pid2 = 0;
                newPage(&pid2, page2);
                LeafNodeInt *node2 = (LeafNodeInt*)page2;
                split(curr_node, node2, pid2);
                *newEntry = new PageKeyPair;
                (*newEntry)->set(pid2, node2->keyArray[0]);
                curr_node->sorted = true;
                node2->sorted = true;
                this->bufMgr->unPinPage(this->file, pid2, true);
                return;
            }                 
        }
    }


    void BTreeIndex::insertEntry(const void *key, const RecordId rid) 
    {
        Page *curr_page;
        PageId curr_id = this->rootPageNum;
        this->bufMgr->readPage(this->file, curr_id, curr_page);
        RIDKeyPair *entry = new RIDKeyPair;
        entry->set(rid, *(int*)key);
        PageKeyPair *newEntry = NULL;
        insertEntry(curr_page, entry, &newEntry, false);
        this->bufMgr->unPinPage(this->file, curr_id, true);
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::startScan (helper functions used by startScan included)
    // -----------------------------------------------------------------------------

    void BTreeIndex::scanSearch(int key, PageId &page_id) { 
        //search through non-leaf nodes
        Page *curr_page;
        PageId last_id = 0;
        PageId curr_id = this->rootPageNum; 
        this->bufMgr->readPage(this->file, curr_id, curr_page);
        NonLeafNodeInt *curr_node = (NonLeafNodeInt*)curr_page;
        while (curr_node->level != 0) {
            last_id = curr_id;
            curr_id = kb_search(curr_node, key); 
            this->bufMgr->unPinPage(this->file, last_id, false);
            this->bufMgr->readPage(this->file, curr_id, curr_page); 
            curr_node = (NonLeafNodeInt*)curr_page;
        }
        //do one more search to enter leaf nodes
        last_id = curr_id;
        page_id = kb_search(curr_node, key);
        this->bufMgr->unPinPage(this->file, last_id, false); 
    }

    void BTreeIndex::existKeys(PageId low_pid, PageId high_pid, int low, int high) {
        bool found = false;
        PageId last_pid = 0;
        PageId curr_pid = low_pid;
        LeafNodeInt *curr_node;
        Page *curr_page;

        while (curr_pid != 0) {
            if (curr_pid == high_pid) {
                found = true;
                break;
            }
            this->bufMgr->readPage(this->file, curr_pid, curr_page);
            curr_node = (LeafNodeInt*)curr_page;
            last_pid = curr_pid;
            curr_pid = curr_node->rightSibPageNo;
            this->bufMgr->unPinPage(this->file, last_pid, false);
        }

        if (low_pid == high_pid) {
            //nodeDump(low_pid);
            //leafDump();
            std::cout << "low pid: " << low_pid << " high pid: " << high_pid << std::endl;
            //std::cout << "low_pid == high_pid" << std::endl;
            this->bufMgr->readPage(this->file, low_pid, curr_page);
            curr_node = (LeafNodeInt*)curr_page;
            int i = kb_search(curr_node, low);
            int j = kb_search(curr_node, high);
            std::cout << "i: " << i << " j: " << j <<std::endl;
            if (i > 0 && j > 0 && i > j){
                std::cout << "error, low_pid position is greater than high_pid position in ridArray." << std::endl;
                std::cout << "i: " << i << " j: " << j <<std::endl;
                found = false;  
            } else if ((i < -1 && j < -1) || (i < -2 && j < -2) || (i < -2 && j < -1)) {
                std::cout << "both numbers are either below min or over max at the same time." << std::endl;
                found = false;
            }
            this->bufMgr->unPinPage(this->file, low_pid, false); 
        }
        if (!found) {
            throw NoSuchKeyFoundException();
        }
    }

    void BTreeIndex::initiateScan(PageId start_pid){
        // initialize the scan
        this->bufMgr->readPage(this->file, start_pid, this->currentPageData);
        this->nextEntry = kb_search((LeafNodeInt*)this->currentPageData, this->lowValInt); 
        //if first page is empty
        if (((LeafNodeInt*)this->currentPageData)->size == 0) {
            this->nextEntry = 0;
            PageId prev = start_pid;
            start_pid = ((LeafNodeInt*)this->currentPageData)->rightSibPageNo;
            this->bufMgr->unPinPage(this->file, prev, false);
            this->bufMgr->readPage(this->file, start_pid, this->currentPageData);
        } else if (this->nextEntry == -1) {//if low value is smaller than smallest value
            this->nextEntry = 0;
        } 
        this->currentPageNum = start_pid;
        this->scanExecuting = true;
    }

    /**
     * helper function for startScan, make the actual operator
     * always GTE and LTE, reduce the complication of binary search.
     */
    int getOffset(const Operator op){
        if (op == GT){
            return 1;
        } else if (op == LT) {
            return -1;
        } else {
            return 0;
        }
    }

    void BTreeIndex::startScan(const void* lowValParm,
            const Operator lowOpParm,
            const void* highValParm,
            const Operator highOpParm)
    {
        //leafDump();
        if (*(int*)lowValParm > *(int*)highValParm) {
            throw BadScanrangeException();
        }
        if ((lowOpParm != GT && lowOpParm != GTE) || (highOpParm != LT && highOpParm != LTE)) {
            throw BadOpcodesException();
        }
        this->lowOp = lowOpParm;
        this->highOp = highOpParm;
        int low = *((int*)lowValParm);
        int high = *((int*)highValParm);
        int offset1 = getOffset(lowOpParm);
        PageId low_pid = 0;
        scanSearch(low + offset1, low_pid);//search for where low value resides
        int offset2 = getOffset(highOpParm);
        PageId high_pid = 0;
        scanSearch(high + offset2, high_pid);//search for where high value resides
        this->lowValInt = low + offset1;
        this->highValInt = high + offset2;
        if (low_pid == 0 || high_pid == 0){
            std::cout << "error: low_pid or high_pid should not be 0. low_pid: ";
            std::cout << low_pid << " high_pid: " << high_pid << std::endl;
        }
        // see if there are entries between low_pid and high_pid
        std::cout << this->lowValInt << this->highValInt << std::endl; 
        existKeys(low_pid, high_pid, this->lowValInt, this->highValInt);
        initiateScan(low_pid); 
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::scanNext
    // -----------------------------------------------------------------------------

    void BTreeIndex::scanNext(RecordId& outRid) 
    {
        if (!this->scanExecuting){
            throw ScanNotInitializedException();
        }
        LeafNodeInt *curr_node = (LeafNodeInt*)this->currentPageData;
        //if nextEntry reaches outside of ridArray
        if (this->nextEntry >= curr_node->size) {
            if (curr_node->rightSibPageNo == 0) {//if no more sibling pages
                throw IndexScanCompletedException();
            } else {//if there are sibling pages, unpin current page
                PageId tmp = this->currentPageNum;
                this->currentPageNum = curr_node->rightSibPageNo;
                this->bufMgr->unPinPage(this->file, tmp, false);   
            }
            //update current page
            this->bufMgr->readPage(this->file, this->currentPageNum, this->currentPageData);
            this->nextEntry = 0;
            curr_node = (LeafNodeInt*)this->currentPageData;
        }      
        //check if current value of nextEntry is already highValParm, if so, throw completeException
        if (this->nextEntry < curr_node->size) {
            if (curr_node->keyArray[this->nextEntry] > this->highValInt){
                throw IndexScanCompletedException();
            }
        }
        outRid = curr_node->ridArray[this->nextEntry];

        this->nextEntry++;
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::endScan
    // -----------------------------------------------------------------------------
    //
    void BTreeIndex::endScan() 
    {
        if (!this->scanExecuting) {
            throw ScanNotInitializedException(); 
        }
        this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
        this->currentPageNum = 0;
        this->currentPageData = NULL;
        this->nextEntry = 0;
        this->scanExecuting = false;
    }

    // -----------------------------------------------------------------------------
    // helper functions used by constructor
    // -----------------------------------------------------------------------------
    //
    
    void BTreeIndex::findFirstLeaf(PageId *pid){
        Page *curr_page;
        PageId last_id;
        PageId curr_id = this->rootPageNum;
        this->bufMgr->readPage(this->file, curr_id, curr_page);
        NonLeafNodeInt* curr_node = (NonLeafNodeInt*)curr_page;
        while (1){
            last_id = curr_id;
            curr_id = curr_node->pageNoArray[0];
            if (curr_node->level == 0){
                break;
            }
            this->bufMgr->unPinPage(this->file, last_id, false);
            this->bufMgr->readPage(this->file, curr_id, curr_page);
            curr_node = (NonLeafNodeInt*)curr_page;
        }
        // do once more to reach leaf node
        *pid = curr_node->pageNoArray[0];
        this->bufMgr->unPinPage(this->file, last_id, false);
    }

    void BTreeIndex::sortAllLeaf(){
        PageId curr_id = 0;
        findFirstLeaf(&curr_id);
        Page *curr_page;
        this->bufMgr->readPage(this->file, curr_id, curr_page);
        LeafNodeInt* curr_node = (LeafNodeInt*) curr_page;
        bool sorted = false;
        PageId last_id;
        while(1){
            sorted = false;
            // leaf node may be sorted while splitting,
            // and may remain sorted after splitting
            // (e.g.) no insertion happened after splitting
            if (!curr_node->sorted){
                sorted = true;
                if (curr_node->size != 0){
                    qsort(curr_node, 0, curr_node->size - 1);
                }
            }
            if (curr_node->rightSibPageNo == 0){
                this->bufMgr->unPinPage(this->file, curr_id, sorted);
                break;
            }
            last_id = curr_id;
            curr_id = curr_node->rightSibPageNo;
            this->bufMgr->unPinPage(this->file, last_id, sorted);
            this->bufMgr->readPage(this->file, curr_id, curr_page);
            curr_node = (LeafNodeInt*) curr_page;
        }

    }

    void BTreeIndex::openIndexFile(std::string &indexName){
        this->file = new BlobFile(indexName, false);
        this->headerPageNum = file->getFirstPageNo();
        Page *metaInfoPage;
        IndexMetaInfo *metaInfo;
        this->bufMgr->readPage(this->file, this->headerPageNum, metaInfoPage);
        metaInfo = (IndexMetaInfo*)metaInfoPage;
        if (metaInfo->attrByteOffset != this->attrByteOffset){
            std::cout << "Attribute byte offset does not match, use original attribute byte offset" << std::endl;
            this->attrByteOffset = metaInfo->attrByteOffset;
        }
        if (metaInfo->attrType != this->attributeType) {
            std::cout << "Attribute type does not match, use original attribute type" << std::endl;
            this->attributeType = metaInfo->attrType;
        }
        this->rootPageNum = metaInfo->rootPageNo;
        this->bufMgr->unPinPage(this->file, this->headerPageNum, false);  
    }

    void BTreeIndex::createIndexFile(std::string &indexName, const std::string &relationName){
        this->file = new BlobFile(indexName, true);
        Page *metaInfoPage = NULL;
        Page *rootNodePage = NULL;
        Page *firstLeafPage = NULL;
        Page *secondLeafPage = NULL;
        PageId first_leaf_pid = 0;
        PageId second_leaf_pid = 0;
        IndexMetaInfo *metaInfo;

        newPage(&this->headerPageNum, metaInfoPage);
        newPage(&this->rootPageNum, rootNodePage);
        newPage(&first_leaf_pid, firstLeafPage);
        newPage(&second_leaf_pid, secondLeafPage);

        LeafNodeInt* first_leaf_node = (LeafNodeInt*)firstLeafPage;
        first_leaf_node->rightSibPageNo = second_leaf_pid;
        this->bufMgr->unPinPage(this->file, first_leaf_pid, true); 
        this->bufMgr->unPinPage(this->file, second_leaf_pid, true); 

        metaInfo = (IndexMetaInfo*)metaInfoPage;
        strcpy(metaInfo->relationName, relationName.c_str());
        metaInfo->attrByteOffset = this->attrByteOffset;
        metaInfo->attrType = this->attributeType;
        metaInfo->rootPageNo = this->rootPageNum;
        NonLeafNodeInt *root = (NonLeafNodeInt*)rootNodePage;
        root->pid = this->rootPageNum;
        root->size = 0;
        root->level = 0;
        root->pageNoArray[0] = first_leaf_pid;
        root->pageNoArray[1] = second_leaf_pid;
        this->bufMgr->unPinPage(this->file, this->headerPageNum, true); 
        this->bufMgr->unPinPage(this->file, this->rootPageNum, true); 
        this->createExecuting = true;
        FileScan *s = new FileScan(relationName, this->bufMgr);

        try {
            //use fileScan to get record, then insert into the tree    
            RecordId rid;
            std::string record;
            void* key;
            while(1){
                s->scanNext(rid);
                record = s->getRecord();
                key = (void*)(record.c_str() + attrByteOffset);
                insertEntry(key, rid);
            }
        } catch (EndOfFileException e){
            sortAllLeaf();
            std::cout << "file scan done" << std::endl;
        }
        this->bufMgr->flushFile(this->file);
        this->createExecuting = false;
        delete(s);
    }

    void BTreeIndex::newPage(PageId *pid, Page *&page){
        this->bufMgr->allocPage(this->file, *pid, page);
        memset(page, 0, Page::SIZE);
    }

    // -----------------------------------------------------------------------------
    // dubug functions
    // -----------------------------------------------------------------------------
    //
    void BTreeIndex::leafDump(){
        if (debug){
            PageId curr_id;
            findFirstLeaf(&curr_id);
            Page *curr_page;
            PageId last_id;
            this->bufMgr->readPage(this->file, curr_id, curr_page);
            LeafNodeInt* curr_node;
            int i = 0;
                while(1){
                    curr_node = (LeafNodeInt*)curr_page;
                    i++;
                    std::cout << "[";
                    for (int i = 0; i < curr_node->size; i++){
                        std::cout << "(" << curr_node->keyArray[i] << "," << curr_node->ridArray[i].page_number << ")";
                    }
                    std::cout << "]" << std::endl;
                    last_id = curr_id;
                    curr_id = curr_node->rightSibPageNo;
                    if (curr_id == 0){
                        break;
                    }
                    this->bufMgr->unPinPage(this->file, last_id, false);
                    this->bufMgr->readPage(this->file, curr_id, curr_page);
                }
            this->bufMgr->unPinPage(this->file, last_id, false);
            std::cout << "total leaf node: " << i << std::endl;
        }
    }

    void BTreeIndex::rootDump(){
        if (!debug){
            Page *curr_page;
            this->bufMgr->readPage(this->file, this->rootPageNum, curr_page);

            NonLeafNodeInt* curr_node = (NonLeafNodeInt*)curr_page;
            std::cout << "root: "<< curr_node->pid <<" , level: " << curr_node->level << " size: " << curr_node->size << std::endl;
            std::cout << "[";
            for (int i = 0; i < curr_node->size; i++){
                std::cout << "(" << curr_node->keyArray[i] << "," << curr_node->pageNoArray[i] << ")";
            }
            std::cout << "(," << curr_node->pageNoArray[curr_node->size] << ")";
            std::cout << "]" << std::endl;

            this->bufMgr->unPinPage(this->file, this->rootPageNum, false);
        }
    }


    void BTreeIndex::nodeDump(PageId pid){
        if (debug){
            Page *curr_page;
            this->bufMgr->readPage(this->file, pid, curr_page);

            NonLeafNodeInt* curr_node = (NonLeafNodeInt*)curr_page;
            std::cout << "page pid: " << curr_node->pid << " level: " << curr_node->level << std::endl;
            std::cout << "[";
            for (int i = 0; i < curr_node->size; i++){
                std::cout << "(" << curr_node->keyArray[i] << "," << curr_node->pageNoArray[i] << ")";
            }
            std::cout << "(," << curr_node->pageNoArray[curr_node->size] << ")";
            std::cout << "]" << std::endl;

            this->bufMgr->unPinPage(this->file, pid, false);
        }
    }
}
/*
if (debug){
    std::cout << "==========================================================================" << std::endl;
    nodeDump(curr_node->pid);
}
if (debug){
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    nodeDump(curr_node->pid);
    std::cout << "==========================================================================" << std::endl;
    leafDump();
    std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
}
if (debug){
    std::cout << "==========================================================================" << std::endl;
    nodeDump(curr_node->pid);
}
if (debug){
    std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << std::endl;
    nodeDump(curr_node->pid);
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    nodeDump(node2->pid);
    std::cout << "==========================================================================" << std::endl;
}
*/
