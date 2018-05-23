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

#pragma once

#include <iostream>
#include <string>
#include "string.h"
#include <sstream>

#include "types.h"
#include "page.h"
#include "file.h"
#include "buffer.h"

namespace badgerdb
{

    /**
     * @brief Datatype enumeration type.
     */
    enum Datatype
    {
        INTEGER = 0,
        DOUBLE = 1,
        STRING = 2
    };

    /**
     * @brief Scan operations enumeration. Passed to BTreeIndex::startScan() method.
     */
    enum Operator
    { 
        LT, 	/* Less Than */
        LTE,	/* Less Than or Equal to */
        GTE,	/* Greater Than or Equal to */
        GT		/* Greater Than */
    };


    /**
     * @brief Number of key slots in B+Tree leaf for INTEGER key.
     */
    //                                          sorted or not     number of keys       sibling ptr             key               rid
    const  int INTARRAYLEAFSIZE = ( Page::SIZE - sizeof( bool ) - sizeof( int ) - sizeof( PageId ) ) / ( sizeof( int ) + sizeof( RecordId ) );

    /**
     * @brief Number of key slots in B+Tree non-leaf for INTEGER key.
     */
    //                                        page id of this page     number of keys     level     extra pageNo                  key       pageNo
    const  int INTARRAYNONLEAFSIZE = ( Page::SIZE - sizeof( PageId ) - sizeof( int ) - sizeof( int ) - sizeof( PageId ) ) / ( sizeof( int ) + sizeof( PageId ) );

    /**
     * @brief Structure to store a key-rid pair. It is used to pass the pair to functions that 
     * add to or make changes to the leaf node pages of the tree. Is templated for the key member.
     */
    //template <class T>
        class RIDKeyPair{
            public:
                RecordId rid;
                int key;
                void set( RecordId r, int k)
                {
                    rid = r;
                    key = k;
                }
        };

    /**
     * @brief Structure to store a key page pair which is used to pass the key and page to functions that make 
     * any modifications to the non leaf pages of the tree.
     */
    //template <class T>
        class PageKeyPair{
            public:
                PageId pageNo;
                int key;
                void set( int p, int k)
                {
                    pageNo = p;
                    key = k;
                }
        };

    /**
     * @brief Overloaded operator to compare the key values of two rid-key pairs
     * and if they are the same compares to see if the first pair has
     * a smaller rid.pageNo value.
     */
    template <class T>
        bool operator<( const RIDKeyPair& r1, const RIDKeyPair& r2 )
        {
            if( r1.key != r2.key )
                return r1.key < r2.key;
            else
                return r1.rid.page_number < r2.rid.page_number;
        }

    /**
     * @brief The meta page, which holds metadata for Index file, is always first page of the btree index file and is cast
     * to the following structure to store or retrieve information from it.
     * Contains the relation name for which the index is created, the byte offset
     * of the key value on which the index is made, the type of the key and the page no
     * of the root page. Root page starts as page 2 but since a split can occur
     * at the root the root page may get moved up and get a new page no.
     */
    struct IndexMetaInfo{
        /**
         * Name of base relation.
         */
        char relationName[20];

        /**
         * Offset of attribute, over which index is built, inside the record stored in pages.
         */
        int attrByteOffset;

        /**
         * Type of the attribute over which index is built.
         */
        Datatype attrType;

        /**
         * Page number of root page of the B+ Tree inside the file index file.
         */
        PageId rootPageNo;
    };

    /*
       Each node is a page, so once we read the page in we just cast the pointer to the page to this struct and use it to access the parts
       These structures basically are the format in which the information is stored in the pages for the index file depending on what kind of 
       node they are. The level memeber of each non leaf structure seen below is set to 1 if the nodes 
       at this level are just above the leaf nodes. Otherwise set to 0.
       */

    /**
     * @brief Structure for all non-leaf nodes when the key is of INTEGER type.
     */
    struct NonLeafNodeInt{
        /**
         * page id of this page
         */
        PageId pid;

        /**
         * number of keys stored in this node
         */
        int size;

        /**
         * Level of the node in the tree.
         */
        int level;

        /**
         * Stores keys.
         */
        int keyArray[ INTARRAYNONLEAFSIZE ];

        /**
         * Stores page numbers of child pages which themselves are other non-leaf/leaf nodes in the tree.
         */
        PageId pageNoArray[ INTARRAYNONLEAFSIZE + 1 ];
    };


    /**
     * @brief Structure for all leaf nodes when the key is of INTEGER type.
     */
    struct LeafNodeInt{
        /**
         * whether the node is sorted or not
         */
        bool sorted;

        /**
         * number of keys stored in this node
         */
        int size;

        /**
         * Stores keys.
         */
        int keyArray[ INTARRAYLEAFSIZE ];

        /**
         * Stores RecordIds.
         */
        RecordId ridArray[ INTARRAYLEAFSIZE ];

        /**
         * Page number of the leaf on the right side.
         * This linking of leaves allows to easily move from one leaf to the next leaf during index scan.
         */
        PageId rightSibPageNo;
    };


    /**
     * @brief BTreeIndex class. It implements a B+ Tree index on a single attribute of a
     * relation. This index supports only one scan at a time.
     */
    class BTreeIndex {

        private:

            /**
             * File object for the index file.
             */
            File		*file;

            /**
             * Buffer Manager Instance.
             */
            BufMgr	*bufMgr;

            /**
             * Page number of meta page.
             */
            PageId	headerPageNum;

            /**
             * page number of root page of B+ tree inside index file.
             */
            PageId	rootPageNum;

            /**
             * Datatype of attribute over which index is built.
             */
            Datatype	attributeType;

            /**
             * Offset of attribute, over which index is built, inside records. 
             */
            int 		attrByteOffset;

            /**
             * Number of keys in leaf node, depending upon the type of key.
             */
            int			leafOccupancy;

            /**
             * Number of keys in non-leaf node, depending upon the type of key.
             */
            int			nodeOccupancy;


            // MEMBERS SPECIFIC TO SCANNING

            /**
             * True if an index scan has been started.
             */
            bool		scanExecuting;

            /**
             * True if creating a new index file
             */
            bool        createExecuting;

            /**
             * Index of next entry to be scanned in current leaf being scanned.
             */
            int			nextEntry;

            /**
             * Page number of current page being scanned.
             */
            PageId	currentPageNum;

            /**
             * Current Page being scanned.
             */
            Page		*currentPageData;

            /**
             * Low INTEGER value for scan.
             */
            int			lowValInt;

            /**
             * Low DOUBLE value for scan.
             */
            double	lowValDouble;

            /**
             * Low STRING value for scan.
             */
            std::string	lowValString;

            /**
             * High INTEGER value for scan.
             */
            int			highValInt;

            /**
             * High DOUBLE value for scan.
             */
            double	highValDouble;

            /**
             * High STRING value for scan.
             */
            std::string highValString;

            /**
             * Low Operator. Can only be GT(>) or GTE(>=).
             */
            Operator	lowOp;

            /**
             * High Operator. Can only be LT(<) or LTE(<=).
             */
            Operator	highOp;

            /**
             * open existing btree index file
             */
            void openIndexFile(std::string &indexName);

            /**
             * create a new btree index file
             */
            void createIndexFile(std::string &indexName, const std::string &relationName);

            /**
             * search for the leaf node where the key resides
             * @param key the key to be searched
             * @param page_id the leaf node where the key resides will be returned in this param
             */
            void scanSearch(int key, PageId &page_id); 
            
            /**
             * validate if there are any keys exist between low page id and high page id
             *
             * @param low_pid low page id
             * @param high_pid high page id
             * @throw NoSuchKeyFoundException; 
             */
            void existKeys(PageId low_pid, PageId high_pid, int low, int high);
        
            /**
             * helper method for insertEntry(const void* key, const RecordId rid)
             *
             * @param node current page
             * @param entry entry to be inserted
             * @param newEntry entry to be returned
             * @param is_leaf whether current page is a leaf node  
             *
             */
            void insertEntry(Page* node, RIDKeyPair *entry, PageKeyPair **newEntry, bool is_leaf); 
            
            /**
             * wrapper function to create a new page and set
             * whole page to 0s
             * 
             * @param pid page id to be returned
             * @param page page to be returned
             */
            void newPage(PageId *pid, Page *&page);

            /**
             * find the page id of first leaf page
             *
             * @param pid return the page id in pid
             *
             */
            void findFirstLeaf(PageId *pid);

            /**
             * sort all leaf nodes, since leaf nodes are inserted
             * in unsorted order.
             */
            void sortAllLeaf();

            /**
             * print all content of all leaf nodes.
             */
            void leafDump();

            /**
             * print content of root node (debug using small number of entries)
             */
            void rootDump();

            /**
             * print content of given non leaf node
             * @param pid page id of leaf node
             */
            void nodeDump(PageId pid);

            /**
             * initialize the global variables for scanning
             *
             * @param start_pid starting page's page id.
             */
            void initiateScan(PageId start_pid);

        /**
         * whether a tree node is full or not, there are overloaded functions 
        * max size is the actual max capacity - 1, to allow easier logic
        * in split()
        *
        * @param node reference of a tree node
        * @return true if full, false if empty
        */
        bool is_full(NonLeafNodeInt *node);

        /**
         * @overload is_full(NonLeafNodeInt &node)
         */
        bool is_full(LeafNodeInt *node);
   
        /**
         * binary search a key array
         *
         * @param arr pointer to an array
         * @param size size of given array
         * @param key the key to be searched
         * @return the index i that Ki <= key < Ki+1
         */
        int b_search(int *arr, int size, int key);

        /**
         * binary search a tree node's key array, and returns the
         * reference of PageId, which represents the page whose key
         * values must include the value of the "key" parameter.
         * 
         * @param node a reference to a non leaf node
         * @param key the key to search through
         * @return the PageId pointer, the PageId represents next tree node page
         * return 1 if node is empty (pid 1 should be metapage, so it is impossible 
         * to show up as nonleaf page's pid)
         */
        PageId kb_search(NonLeafNodeInt *node, int key);
    
        /**
         * @overload kb_search(NonLeafNodeInt &node, int key)
         * @return the index of recordId in ridArray, which 
         * represents a record from data file, if key is smaller than
         * smallest, return -1; if key is greater than largest, return -2;
         */
        int kb_search(LeafNodeInt *node, int key);
    
        /**
         * insert key and pid into a nonleaf node, while maintaining the 
         * sorted order of keyArray and pageNoArray
         *
         * @param node pointer to a node
         * @param key to be added
         * @param pid page id to be added
         */
        void add_key(NonLeafNodeInt* node, int key, PageId pid);
    
        /**
         * overload add_key for leaf nodes, add without sorting
         *
         */
        void add_key(LeafNodeInt* curr_node, RIDKeyPair *entry);
    
        /**
         * insert key into leaf node while maintaining the internal order
         */
        void insert_key(LeafNodeInt* node, RIDKeyPair *entry);
    
        /**
         * split one nonleaf node into two nonleaf nodes
         *
         * @param node the node to be splitted
         * @param node2 a new node with about half of node's size;
         */
        void split(NonLeafNodeInt* node, NonLeafNodeInt* node2);
    
        /**
         * overload split() for leaf nodes, also links sibling leaf nodes
         */
        void split(LeafNodeInt* node, LeafNodeInt* node2, PageId pid2);
    
        /**
         * method used by qsort(), generate random pivot in
         * range [left, right] (inclusive)
         *
         * @param left min value of a random number
         * @param right max value of a random number
         * @return a random number
         */
        int ran(int left, int right); 
    
        /**
         * parallel quick sort a leaf node, compare on keys, but
         * sort on key-recordId pair.
         * reference: www.algolist.net/Algorithms/Sorting/Quicksort
         *
         * @param node the leaf node to be sorted
         * @param left start index for sorting
         * @param right end index for sorting
         */
        void qsort(LeafNodeInt* node, int left, int right); 

        public:

            /**
             * BTreeIndex Constructor. 
             * Check to see if the corresponding index file exists. If so, open the file.
             * If not, create it and insert entries for every tuple in the base relation using FileScan class.
             *
             * @param relationName        Name of file.
             * @param outIndexName        Return the name of index file.
             * @param bufMgrIn						Buffer Manager Instance
             * @param attrByteOffset			Offset of attribute, over which index is to be built, in the record
             * @param attrType						Datatype of attribute over which index is built
             * @throws  BadIndexInfoException     If the index file already exists for the corresponding attribute, but values in metapage(relationName, attribute byte offset, attribute type etc.) do not match with values received through constructor parameters.
             */
            BTreeIndex(const std::string & relationName, std::string & outIndexName,
                    BufMgr *bufMgrIn,	const int attrByteOffset,	const Datatype attrType);


            /**
             * BTreeIndex Destructor. 
             * End any initialized scan, flush index file, after unpinning any pinned pages, from the buffer manager
             * and delete file instance thereby closing the index file.
             * Destructor should not throw any exceptions. All exceptions should be caught in here itself. 
             * */
            ~BTreeIndex();


            /**
             * Insert a new entry using the pair <value,rid>. 
             * Start from root to recursively find out the leaf to insert the entry in. The insertion may cause splitting of leaf node.
             * This splitting will require addition of new leaf page number entry into the parent non-leaf, which may in-turn get split.
             * This may continue all the way upto the root causing the root to get split. If root gets split, metapage needs to be changed accordingly.
             * Make sure to unpin pages as soon as you can.
             * @param key			Key to insert, pointer to integer/double/char string
             * @param rid			Record ID of a record whose entry is getting inserted into the index.
             **/
            void insertEntry(const void* key, const RecordId rid);


            /**
             * Begin a filtered scan of the index.  For instance, if the method is called 
             * using ("a",GT,"d",LTE) then we should seek all entries with a value 
             * greater than "a" and less than or equal to "d".
             * If another scan is already executing, that needs to be ended here.
             * Set up all the variables for scan. Start from root to find out the leaf page that contains the first RecordID
             * that satisfies the scan parameters. Keep that page pinned in the buffer pool.
             * @param lowVal	Low value of range, pointer to integer / double / char string
             * @param lowOp		Low operator (GT/GTE)
             * @param highVal	High value of range, pointer to integer / double / char string
             * @param highOp	High operator (LT/LTE)
             * @throws  BadOpcodesException If lowOp and highOp do not contain one of their their expected values 
             * @throws  BadScanrangeException If lowVal > highval
             * @throws  NoSuchKeyFoundException If there is no key in the B+ tree that satisfies the scan criteria.
             **/
            void startScan(const void* lowVal, const Operator lowOp, const void* highVal, const Operator highOp);


            /**
             * Fetch the record id of the next index entry that matches the scan.
             * Return the next record from current page being scanned. If current page has been scanned to its entirety, move on to the right sibling of current page, if any exists, to start scanning that page. Make sure to unpin any pages that are no longer required.
             * @param outRid	RecordId of next record found that satisfies the scan criteria returned in this
             * @throws ScanNotInitializedException If no scan has been initialized.
             * @throws IndexScanCompletedException If no more records, satisfying the scan criteria, are left to be scanned.
             **/
            void scanNext(RecordId& outRid);  // returned record id


            /**
             * Terminate the current scan. Unpin any pinned pages. Reset scan specific variables.
             * @throws ScanNotInitializedException If no scan has been initialized.
             **/
            void endScan();

    };

}
