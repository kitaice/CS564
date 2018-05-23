#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <fstream>

class node
{
public:
  node* left;
  node* right;
  char* word;
  //list* count;
  int* occurs;
};
node* root = new node;


/*

class ArrayList {

public:
    // The size we use to increment arrays. I use a small increment for testing.
    const static int ArrayIncrement = 2;

    // Constructor to initialize the list.
    ArrayList();

    // Destructor to clean up the list.
    ~ArrayList();

    // Empties out the array list structure.
    void MakeEmpty( );

    // Tests to see if the array list structure is empty.
    bool isEmpty( );

    // Returns the size of the array.
    int listSize() { return nelts; }

    // Finds a specifies element of the array list. Returns -1 if not found.
    int Find( double y );

    // Inserts an element at a specified position in the array list.
    void Insert( double y, int position );

    // Deletes a specified element in the array list.  ASSIGNED AS LAB
    void Delete( double y );

    // Returns the kth element in the array. Returns 0 if not found.  ASSIGNED AS LAB
    double FindKth( int position );

    // You get this one for free if you implement FindKth.
    double operator[] (int position ) { return FindKth( position ); }

    // Displays the list.  This is a good debugging aid
    void PrintList( );

private:

    int ArraySize;// Size of the array.
    int nelts; // Number of elements in the array.
    double *array;// Pointer to the array.
    
    // Makes sure that there is space for a new element in the list.
    void MakeSpace( );
};

// Functions to support list operations on an array.
#include 
#include 
#include "ArrayList.h"
using namespace std;

// Contructor sets valriables to indicate that we have an empty list.
ArrayList::ArrayList() {

    ArraySize = 0;
    nelts = 0;
    array = NULL;
}
// Destructor used to clean up allocated resources/
ArrayList::~ArrayList() {

    delete [] array;
}
// Delete all the elements from the list.
void ArrayList::MakeEmpty( )
{
    delete [] array;
    nelts = 0;
    ArraySize = 0;
    array = NULL;
}
// Tests to see if there are no elements in the array.
bool ArrayList::isEmpty( )
{
    return nelts == 0;
}
// Return the index of the first occurrance of a specified list element.  Returns -1 if not found.
int ArrayList::Find( double y )
{
    for( int i = 0; i < nelts; i++ ) {
        if( array[i] == y ) {

            // The user is considering the first element to be at position 1.
            return i +1;
        }
    }
    return -1;
}
// Insert the specied number in the list position.
void ArrayList::Insert( double y, int position )
{
    // Abort the program if a bad position is specified.
    assert( position >= 1 && position <= nelts + 1 );

    // Note the array position.  Recall that lists are numbered starting with 1.
    int ia = position - 1;

    // Make sure there is space in the array for a new element.
    MakeSpace( );

    // If we are adding a new element to the list, open up space if necessary.
    if( ia < nelts ) {
        memmove( &array[ia + 1], &array[ia], (nelts - ia) * sizeof(double ) );
    }
    array[ia] = y;
    nelts++;
}
// Display the elements of the list.
void ArrayList::PrintList( )
{
    // Display the list.
    for( int i = 0; i < nelts; i++ ) {
        cout << i + 1<< ". " << array[i] << endl;
    }
}
void ArrayList::MakeSpace( )
{
    // If there is space for another array element, return.
    if( nelts < ArraySize ) return;

    // Create storage to make a larger array.
    double *tmp;
    tmp = new double[ArraySize + ArrayIncrement];

    // Note that exceptions now occur when new fails unless you tell the compiler otherwise.
    assert( tmp != NULL );

    // If the previous array was not empty, must copy its contents.
    if( nelts > 0 ) {
        memcpy( tmp, array, nelts * sizeof(double) );
        delete [] array;
    }
    ArraySize += ArrayIncrement;
    array = tmp;
}
*/
class list
{
}


class bst
{
private:
  node* root;
  node* insert(node* oldnode, node* newnode, int occurence);
public:
  int* search(std::string s,int order);
  void insert(node* newnode, int occurence){
    root = insert(root, newnode, occurence);
  }

};
/*
void BST::find(int item, node **par, node **loc)
{
    node *ptr, *ptrsave;
    if (root == NULL)
    {
        *loc = NULL;
        *par = NULL;
        return;
    }
    if (item == root->info)
    {
        *loc = root;
        *par = NULL;
        return;
    }
    if (item < root->info)
        ptr = root->left;
    else
        ptr = root->right;
    ptrsave = root;
    while (ptr != NULL)
    {
        if (item == ptr->info)
        {
            *loc = ptr;
            *par = ptrsave;
            return;
        }
        ptrsave = ptr;
        if (item < ptr->info)
            ptr = ptr->left;
	else
	    ptr = ptr->right;
    }
    *loc = NULL;
    *par = ptrsave;
}
*/
int* bst::search(std::string s, int order){
}

node* bst::insert(node* oldnode, node* newnode, int occurence){
  if(root == NULL){
    root = new node;
    root->word = newnode->word;
    root->occurs = newnode->occurs;
    root->left = NULL;
    root->right = NULL;
  }
  int result = strcmp(oldnode->word,newnode->word);
  if(result==0){
    int arraylen=sizeof(oldnode->occurs)/sizeof(unsigned int*);
    int *newarray=new int[arraylen+1];
    for(int i=0;i<arraylen;i++)newarray[i]=oldnode->occurs[i];
    newarray[arraylen]=occurence;
    delete[] oldnode->occurs;
    oldnode->occurs=newarray;
    delete(newnode);
  }
  if(result>0){
      oldnode=insert(oldnode->left,newnode,occurence);
  }
  else{
      insert(oldnode->right,newnode,occurence);
  }
  return oldnode;
}
