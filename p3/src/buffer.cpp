/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB, augmented by Cornard Chen, Jack Chen, Yudong Huang.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb { 

    //----------------------------------------
    // Constructor of the class BufMgr
    //----------------------------------------

    BufMgr::BufMgr(std::uint32_t bufs): numBufs(bufs) {
        bufDescTable = new BufDesc[bufs];

        for (FrameId i = 0; i < bufs; i++) 
        {
            //std::cout << "initializing " << i << "th bufDescTable" << std::endl;
            bufDescTable[i].frameNo = i;
            bufDescTable[i].valid = false;
        }

        bufPool = new Page[bufs];

        int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
        hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

        clockHand = bufs - 1;
    }

    //TODO
    BufMgr::~BufMgr() {

        delete hashTable;
        // delete hashtable
        delete[] bufPool;
        // delete bufpool
        for(unsigned int i=0; i<numBufs; i++){
            BufDesc *tmp = &(bufDescTable[i]);
            if(tmp->dirty && tmp->valid){
                //write the frame back to page, maybe writePage maybe flushFile
                tmp->file->writePage(bufPool[i]);
                bufDescTable[i].Clear();
            }
        }
        delete[] bufDescTable;
        // clear bufDescTable
    }
    //TODO
    void BufMgr::advanceClock()
    {
        clockHand=(clockHand+1)%numBufs;//increase one and mod the size of buffers
    }
    //TODO
    void BufMgr::allocBuf(FrameId & frame)
    {
        //std::cout << "in allocBuf" << std::endl;
        std::uint32_t i = 0;
        std::uint32_t counter = 0;
        bool full = false;
        while (1){
            i = clockHand;
            //std::cout<<"line 73, clockHand is: "<< i << std::endl;
            BufDesc *tmp = &(bufDescTable[i]);
            //when buffer is full
            if (full) {
                //std::cout<<"buffer exceed" << std::endl;
                throw BufferExceededException();
            }
            if (tmp->pinCnt > 0) {
                //record number of used frame
                counter ++;
            } else if (tmp->refbit) {
                //if the frame is referenced recently
                tmp->refbit = false;
            } else {
                //the frame is not pinned and not referenced recently
                //so it can be replaced
                frame = i;
                advanceClock();
                if (tmp->dirty && tmp->valid) {//write back if it is a valid dirty page
                    tmp->file->writePage(bufPool[i]);
                }
                if (tmp->valid){//remove record of the valid page from hash table
                    //std::cout << "removing from hash table" << std::endl;
                    tmp->valid = false;
                    //std::cout << "removing: " << (void*)tmp->file << "," << tmp->pageNo << std::endl;
                    hashTable->remove(tmp->file, tmp->pageNo); 
                }
                return;
            }
            advanceClock();
            if (counter >= numBufs){
                full = true;
            }
        } 
    }

    //TODO
    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        FrameId frameId=0;
        try{
            hashTable->lookup(file, pageNo, frameId);//if there is a frame in buffer
            bufDescTable[frameId].pinCnt += 1;
            bufDescTable[frameId].refbit = true;
            page = &bufPool[frameId];
	    return;
        } catch (HashNotFoundException e) {//if the page is not present
            //std::cout<<"buffer::readPage::did not found in hash table"<<std::endl;
            allocBuf(frameId);
            bufPool[frameId] = file->readPage(pageNo);
            //bufDescTable[frameId].pinCnt += 1;  // doubt
            bufDescTable[frameId].refbit = true;
            hashTable->insert(file, pageNo, frameId);
            bufDescTable[frameId].Set(file,pageNo);
            page = &bufPool[frameId];
        }

        //std::cout << "leaving readPage" << std::endl;
    }

    //TODO
    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty)
    {
        //std::cout << "entering unPinPage" << std::endl;
        FrameId frameN = 0;
        try{
            BufMgr::hashTable->lookup(file, pageNo, frameN);
        } catch (HashNotFoundException e){
            return;
        }
        BufDesc *tmp = &(bufDescTable[frameN]);
        if (tmp->pinCnt == 0){
            throw PageNotPinnedException(file->filename(), pageNo, frameN);
        } else {
            tmp->pinCnt--;
        }
        if (dirty) {
            tmp->dirty = true;
        } else {
	    tmp->dirty = false;
	}
        //std::cout << "leaving unPinPage, pinCnt is: " << tmp->pinCnt << std::endl;
    }

    //bufPool and bufDescTable user identical indexes
    void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page)
    {
        FrameId frameNo = 0;
        allocBuf(frameNo); 
        bufPool[frameNo] = file->allocatePage();//put the allocated page in the pool
        page = &bufPool[frameNo];
        pageNo = page->page_number();
        BufDesc *tmp = &(bufDescTable[frameNo]);
        //std::cout << "original: " << tmp->
        tmp->Set(file, pageNo);
        tmp->frameNo = frameNo;
        BufMgr::hashTable->insert(file, pageNo, frameNo);
        //std::cout << "leaving allocPage" << std::endl;
    }
    //TODO
    void BufMgr::flushFile(const File* file) 
    {
        //traverse the pool and write out pages of this file
        for(unsigned int i=0; i < numBufs; i++){
            BufDesc *tmp = &(bufDescTable[i]);
            if(tmp->file==file){
                if(!tmp->valid) throw BadBufferException(tmp->frameNo, tmp->pageNo, tmp->dirty, tmp->valid);
                if(tmp->pinCnt >= 1) 
                    throw PagePinnedException(tmp->file->filename(), tmp->pageNo, tmp->frameNo);
                if(tmp->dirty){
                    tmp->file->writePage(bufPool[i]);
                    tmp->dirty= false;
                }
                hashTable->remove(tmp->file, tmp->pageNo);
                tmp->Clear();//after everything done, clear the frame record
            }
        }
        //std::cout << "leaving flushFile" << std::endl;
    }
    //TODO
    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        //std::cout << "entering disposePage" << std::endl;
        std::uint32_t frameNo = 0;
        try{
            BufMgr::hashTable->lookup(file, PageNo, frameNo);
        } catch (HashNotFoundException e){
            __throw_exception_again;
        }
        if (frameNo > 0) {
            BufDesc *tmp = &(bufDescTable[frameNo]);
            tmp->Clear();
            BufMgr::hashTable->remove(file, PageNo);
        }
        file->deletePage(PageNo);

        //std::cout << "leaving disposePage" << std::endl;
    }

    void BufMgr::printSelf(void) 
    {
        BufDesc* tmpbuf;
        int validFrames = 0;

        for (std::uint32_t i = 0; i < numBufs; i++)
        {
            tmpbuf = &(bufDescTable[i]);
            std::cout << "FrameNo:" << i << " ";
            tmpbuf->Print();

            if (tmpbuf->valid == true)
                validFrames++;
        }

        std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
    }

}
