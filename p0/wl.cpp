//
// File: wl.h
//
//  Description: Add stuff here ...
//  Student Name: Conrad Chen
//  UW Campus ID: 9071560800
//  email: wchen283@wisc.edu

/*!
    \file
 */
#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <fstream>
#include "wl.h"
std::string errormsg ("ERROR: Invalid command");



/*!
  definition of the bst clear method
*/
void bst::clear(node* parent){
    if(parent==NULL) return;  //! if the parent node has been removed
    if(parent->left==NULL&&parent->right==NULL){
      delete[] parent->loc;
      delete[] parent->word;
      delete parent;
      return;
    } //! if the children of the parent have been removed
    clear(parent->left);  //!remove the left child
    parent->left=NULL;
    clear(parent->right); //!remove the right child
    parent->right=NULL;
    delete[] parent->loc;
    delete[] parent->word;
    delete parent;

    return;
}

/*!
  definition of the bst search method
*/
int bst::search(node* parent, std::string s, int order){
  if(parent == NULL) return -1;
  int result = strcmp(parent->word,s.c_str());
  //!compare the target string and the string in current node
  if(result < 0){

    return search(parent->right, s, order);
  } //!if current string is smaller than the target
  else if(result > 0){

    return search(parent->left, s, order);
  } //!if current string is larger than the target
  else{

    if(order > parent->occurs) return -1;
    return parent->loc[order-1];
  } //!if found the target
}

/*!
  definition of the bst insert method
*/
node* bst::insert(node* parent, node* newnode){
  if(parent == NULL){

    parent = newnode;

    parent->word = newnode->word;

    parent->loc = newnode->loc;

    parent->left = NULL;

    parent->right = NULL;

    parent->occurs=1;

    return parent;
  } //!for starting at empty root
    int occurence=newnode->loc[0];

    int result = strcmp(parent->word,newnode->word);
    if(result<0) {//!if parent is smaller than newnode

      parent->right=insert(parent->right,newnode);

    } else if(result>0) {//!if parent is larger than newnode

      parent->left=insert(parent->left,newnode);

    } else {//!if newnode duplicate of parent

      int arraylen=parent->occurs+1;
      int *newarray=new int[arraylen];
      for(int i=0;i<arraylen-1;i++) newarray[i]=parent->loc[i];
      newarray[arraylen-1]=occurence;
      delete[] newnode->loc;
      newnode->loc=NULL;
      delete[] newnode->word;
      newnode->word=NULL;
      delete[] parent->loc;
      parent->loc=newarray;
      parent->occurs++;
      delete newnode;
    }
    return parent;
}


bst *tree=new bst;/*!< the tree that will be used in the program*/

//!trim the command line
/*!
 * \param s string to be trimmed
 * \return a long string
 */

std::string trim(std::string s){
  if(s.empty()) return s;
  // erase first not of space and tab
  s.erase(0,s.find_first_not_of(" \t"));
  // erase last not of space nad tab
  s.erase(s.find_last_not_of(" \t")+1);
  return s;
}

//! check the type of a char
/*!
 * \param c char to be checked
 * \return 0 for invalid char, 1 for number,
 * 2 for lower case, 3 for upper case, 4 for '
 */
int checktype(char c){
  if(c == 39) return 4;
  else if(c >= 48 && c<= 57) return 1;
  else if(c >= 65 && c<= 90) return 3;
  else if(c >= 97 && c<= 122) return 2;
  else return 0;
}

//!case char to integer
/*!
 * \param s string to be modified
 * \return integer form
 */
int toint(std::string s){
  std::istringstream iss(s);
  int num;
  iss>>num;
  return num;
}

//! check number validity
/*!
 * \param num number to be checked 
 * \return 0 if invalid, 1 valid
 */

int checknum(int num){
  if (num > 0) return 1;
  return 0;
}

//!change a token into lower case if it is upper case
/*!
 * \param s the string to check
 * \return void
 */
std::string tolower(std::string s){
  int slength=s.length();
  char character,exg;
  std::stringstream stream;
  for(int i=0; i< slength;i++) {
    character = s.at(i);
    stream.str("");
    if(checktype(character)==3) {//!if found to be upper case
      exg = (char)((int)character + 32);
      stream<<exg;
      s.replace(i,1,stream.str());
    }
  }
  return s;
}

//!check any char in the string is of expected type
/*!
 * \param s the string to check
 * \return true if yes
 */
bool expectedtype(std::string s,int strlen,int type){
  for(int i=0;i<strlen;i++){
    if(checktype(s.at(i))==type){
      return true; //!a char of expected type found
    }
  }
  return false;
}

/*!
 * read the file and load into the tree
 */
void loadtext(std::string file){
  std::ifstream infile(file.data());
  if(!infile.is_open()) {
    std::cout<<errormsg<<std::endl;
    
  } else {
    tree->clear();
    std::stringstream stream;
    infile>>std::noskipws;
    int order=0;
    while(!infile.eof()){
      char character;
      infile>>character;
      if(checktype(character)!=0){
        stream<<character;
      } else {
        std::string buf = stream.str();
        std::string word = tolower(buf);
        if(word!="" && word !="\n"){
          order++;
          char *realword=new char[word.length()+1];
          strcpy(realword, word.c_str());
          //!insert word
          node* newnode = new node;
          newnode->word=realword;
          newnode->left=NULL;
          newnode->right=NULL;
          newnode->occurs=1;
          newnode->loc=new int[1];
          newnode->loc[0]=order;
          tree->insert(newnode);
        }
        stream.str("");
      }
    }
  }
}
//!repeatedly print prompt line and let user input command
/*!
 * \return void
 */
void loop(){
  while(1){
    std::cout<<">";

    std::string input;
    std::string trimmed;
    std::getline(std::cin, input);
    //! trim the command
    trimmed = trim(input);
    std::istringstream iss(trimmed);
    std::string tokens[3];
    std::string tmptk[3];
    //! split into tokens
    for(int i=0;i<3;i++){
      std::getline(iss,tmptk[i],' ');
      if(iss.rdbuf()->in_avail() == 0) {
        break;
      }
      //std::cout<<tokens[i]<<std::endl;
    }

    if(iss.rdbuf()->in_avail() == 0){
      int tokenlength = tmptk[0].length();

    //! make token to lower case
      for(int i=0;i<3;i++){
        tokens[i]=tolower(tmptk[i]);
      }
     
      if(tokens[0] == "end") {
        if((!tokens[1].empty())||(!tokens[2].empty())) {
          std::cout<<errormsg<<std::endl;//!more cmd than needed
          continue;
        }
        break;
      }
      else if(tokens[0] == "new"){
        if((!tokens[1].empty())||(!tokens[2].empty())) {
          std::cout<<errormsg<<std::endl;//!more cmd than needed
          continue;
        }
        tree->clear();
      } else if(tokens[0] == "load") {
        if(!tokens[2].empty()) {
          std::cout<<errormsg<<std::endl;//!more cmd than needed
          continue;
        }
        loadtext(tokens[1]);
      } else if(tokens[0] == "locate") {
        if(tokens[2].empty()){
          std::cout<<errormsg<<std::endl;//!third token is null
          continue;
        }
        tokenlength=tokens[1].length();
        if(expectedtype(tokens[1],tokenlength,0)==true) {
          std::cout<<errormsg<<std::endl;
          continue;
        }//!check any invalid character
        tokenlength=tokens[2].length();
        if(expectedtype(tokens[2],tokenlength,0)||expectedtype(tokens[2],tokenlength,2)||expectedtype(tokens[2],tokenlength,3)||expectedtype(tokens[2],tokenlength,4)) {
          std::cout<<errormsg<<std::endl;
          continue;
        }//!check if any of the char is not number, or it is a negative number

        //!convert str to int
        int n = toint(tokens[2]);
        if(checknum(n)!=1) {
          std::cout<<errormsg<<std::endl;
          continue;
        }
        int match = tree->search(tokens[1],n);
        if(match!=-1){
          std::cout<<match<<std::endl;
        } else {
          std::cout<<"No matching entry"<<std::endl;
        }
      } else std::cout<<errormsg<<std::endl;

    } else std::cout<<errormsg<<std::endl;//!invalid command since too long
  }
}

int main(){
  //!infinite loop for user interface
  loop();
  tree->clear();
  delete(tree);
  return 0;
}

