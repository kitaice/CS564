//
// File: wl.h
// 
//  Description: Add stuff here ...
//  Student Name: Conrad Chen
//  UW Campus ID: 9071560800
//  email: wchen283@wisc.edu
/*!
  The class for a tree node
*/
class node
{
public:
  node* left;    /*!<pointer to the left child.*/
  node* right;   /*!<pointer to the right child.*/
  char* word;    /*!<The word to be stored.*/
  int* loc;      /*!<the location where the nth word occurs in the text.*/
  int occurs;    /*!<how many times the word occurs in the text.*/
};

/*!
   The binary search tree class.
 */
class bst
{
private:
  node* root;    /*!<root of the tree.*/

  //! private method for insert a node into bst.
  /*!
    \sa insert()
    \param parent the parent node.
    \param newnode the new node to insert.
  */
  node* insert(node* parent, node* newnode);
  //! private method for search a word from bst.
  /*!
    \sa search()
    \param parent the node to start searching.
    \param s the word string to search.
    \param order the nth time the s occurs in text.
  */
  int search(node* parent, std::string s,int order);
  //! private method for deleting the nodes.
  /*!
    \sa clear()
    \param parent the node to start clearing from.
  */
  void clear(node* parent);
public:
  //! interface for the private insert method.
  /*!
    \sa insert()
    \param newnode the new node to insert.
  */
  void insert(node* newnode){
    root = insert(root, newnode);
  }
  //! interface for the private search method.
  /*!
    \sa search()
    \param s the word string to search.
    \param order the nth time the s occurs in text.
  */
  int search(std::string s,int order){
    return search(root,s,order);  
  }
  //! interface for the private clear method.
  /*!
    \sa clear()
  */
  void clear(){
    if(root!=NULL) clear(root);
    root = NULL;
  }
};
