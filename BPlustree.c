/*
 *  Created on: Jul 13, 2013
 *      Author: Marcelo
 */

//============================= INCLUDES ===============================
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//============================= CONSTANTS =============================
#define ORDEM 2
#define REGS_POR_PAGINA 2
#define MAX_NAME 20

//---- BTree definition
//#define FILL_FACTOR 0.5
//----------------------
//============================= STRUCTURES =============================
typedef struct Record Record;
typedef struct item item;

struct Record {
  int rnum;
  int key;
  char name[MAX_NAME];
  int age;
};

typedef struct Page Page;

struct Page {
  int fnum;
  int records[REGS_POR_PAGINA];
  int next;
  int stored;
};

typedef struct InternalNode InternalNode;
struct InternalNode {
  InternalNode *pointers[ORDEM * 2 + 1];
  int pages[ORDEM * 2 + 1];
  int index[ORDEM * 2]; //todos recebem -1
  int stored;

  InternalNode* parent;
  bool leaf;

};

struct item {
  InternalNode* node;
  item *next;

  //used for other structures
  int page;
  InternalNode* parent;

};
void write_tree(FILE* fi, InternalNode* root, item** head, item** initial);



//============================= PROTOTYPES =============================
static InternalNode* HEAD;
static int TOTAL_RECORDS_FILE;
static int TOTAL_PAGES_FILE;
static int TOTAL_INDEX_FILE;
void logicalRemove(InternalNode* p, int i);
void insert_in_this_position(InternalNode* root, int b, int iIndex,
    InternalNode* child);
InternalNode* searchKey(InternalNode* root, int key);
InternalNode* searchKeyBPlus(FILE* fp, FILE* fr, InternalNode* root, int key,
    int* i);
void write_page_on_file_usefnum(FILE* f, Page* p);
//======insertion
void insert(InternalNode* node, int b, bool up, InternalNode* childBySplit);
void insert_at_index_with_page(InternalNode* node, int b, bool up,
    int pageChildBySplit);
bool remove_action(InternalNode* node, int i/*key in index array*/);
void changeValue(InternalNode* p, int i, int key);
void get_internal_node(FILE *f, InternalNode *r, int offset);
void write_record_file(FILE* f, Record* p);
void print_tree(FILE* fp, FILE* fr, InternalNode* root, int page, item** head,
    item** initial, int* no, int* index);
void get_record(FILE *f, Record *r, int offset);
void get_page(FILE *f, Page *r, int offset);
int get_key_from_rfile(FILE* fr, int offset);
void write_page_on_file_new(FILE* f, Page* p);
void write_page_on_file(FILE* f, Page* p, int* parent_pointer);
void receive_next_and_save(FILE* fp, Page* receiver, int next);
void delete_key_page(FILE* fp, Page* page, int iIndex);
int search_at_record(FILE* fp, FILE* fr, Page* p, int key);
InternalNode* searchKey(InternalNode* root, int key);
int keyIndexFromNode(InternalNode* node, int key);
int nodePageIndexFromNode(InternalNode* parent, Page* pChild);

//====================== GERENCIAMENTO DE ARQUIVO=======================

void write_record_file(FILE* f, Record* p) {
  int e;
  e = fseek(f, TOTAL_RECORDS_FILE * sizeof(Record), SEEK_SET);
  if (e == 0) {
    p->rnum = TOTAL_RECORDS_FILE;
    TOTAL_RECORDS_FILE++;
    fwrite(p, sizeof(Record), 1, f);
  }
}

Record* new_record(FILE* fr, int key) {
  Record* r = (Record*) malloc(sizeof(Record));
  r->key = key;
  scanf("%[^\n]s%*c",r->name);
  scanf("%d%*c", &r->age);
  write_record_file(fr, r);
  return r;
}

InternalNode* new_node(InternalNode* parent) {
  InternalNode* node = (InternalNode*) malloc(sizeof(InternalNode));
  int i;
  for (i = 0; i < ORDEM * 2; i++) {
    node->index[i] = -1;
    node->pages[i] = -1;
    node->pointers[i] = NULL;
  }
  node->pointers[i] = NULL; // last pointer
  node->pages[i] = -1; // last pointer
  node->stored = 0;
  node->parent = parent;
  node->leaf = false; //default

  return node;
}

//==================================Internal Nodes load and print===========================
void add(InternalNode* node, int page, item** head, item** initial) {
  item* current = (item*) malloc(sizeof(item));

  current->node = node;
  current->page = page;

  if ((*initial) == NULL) {
    (*initial) = current;
    (*head) = current;
  } else {
    (*head)->next = current;
    (*head) = (*head)->next;
  }
  current->next = NULL;
}

void add_write(InternalNode* node, item** head, item** initial) {
  item* current = (item*) malloc(sizeof(item));

  current->node = node;

  if ((*initial) == NULL) {
    (*initial) = current;
    (*head) = current;
  } else {
    (*head)->next = current;
    (*head) = (*head)->next;
  }
  current->next = NULL;
}

void add_load(InternalNode* node, item** head, item** initial) {
  item* current = (item*) malloc(sizeof(item));

  current->node = node;

  if ((*initial) == NULL) {
    (*initial) = current;
    (*head) = current;
  } else {
    (*head)->next = current;
    (*head) = (*head)->next;
  }
  current->next = NULL;
}

void store_index(FILE* fi, InternalNode* root) {
  if (root != NULL) {
    item* head = NULL;
    item* initial = NULL;
    add_write(root, &head, &initial);
    while (initial) {
      write_tree(fi, initial->node, &head, &initial);
      initial = initial->next;
    }
  }
}

void write_tree_file(FILE* f, InternalNode* p) {
  int e;
  e = fseek(f, TOTAL_INDEX_FILE * sizeof(InternalNode), SEEK_SET);
  if (e == 0) {
    fwrite(p, sizeof(InternalNode), 1, f);
    TOTAL_INDEX_FILE++;
  }
}

void write_tree(FILE* fi, InternalNode* root, item** head, item** initial) {
  if (!root->leaf) {
    int i = 0;
    while (root->pointers[i] != NULL && i < 2 * ORDEM + 1) {
      add_write(root->pointers[i], head, initial);
      i++;
    }
  }
  write_tree_file(fi, root);
}

void load_tree(FILE* fi, InternalNode* root, item** head, item** initial,
    int* offset) {
  if (!root->leaf) {
    int i;
    for (i = 0; i <= root->stored; i++) {
      InternalNode* son = new_node(root);
      get_internal_node(fi, son, ++(*offset));
      root->pointers[i] = son;
      add_load(root->pointers[i], head, initial);
    }
  }
}

void init_load_tree(FILE* fi) {
  if (TOTAL_INDEX_FILE > 0) {
    InternalNode* root = new_node(NULL);
    get_internal_node(fi, root, 0);
    HEAD = root;

    item* head = NULL;
    item* initial = NULL;
    int offset = 0;
    add_load(root, &head, &initial);
    while (initial) {
      load_tree(fi, initial->node, &head, &initial, &offset);
      initial = initial->next;
    }
  }
}

void init_print_tree(FILE* fp, FILE* fr, InternalNode* root) {
  if (root != NULL) {
    item* head = NULL;
    item* initial = NULL;
    int index = 1;
    int no = 0;
    add(root, -1, &head, &initial);
    while (initial) {
      print_tree(fp, fr, initial->node, initial->page, &head, &initial,
          &no, &index);
      initial = initial->next;
    }

  }
}

void print_tree(FILE* fp, FILE* fr, InternalNode* root, int page, item** head,
    item** initial, int* no, int* index) {
  int i = 0;
  printf("No: %d : ", ++(*no));

  if (page == -1) { //it isn't a page
    if (root->leaf) {
      while (root->pages[i] != -1 && i < 2 * ORDEM + 1) {
        add(NULL, root->pages[i], head, initial);
        printf("apontador: %d ", ++(*index));
        i++;
      }
    } else {
      while (root->pointers[i] != NULL && i < 2 * ORDEM + 1) {
        add(root->pointers[i], -1/* no pages*/, head, initial);
        printf("apontador: %d ", ++(*index));
        i++;
      }
    }
    printf(" ");
    i = 0;
    while (root->index[i] != -1 && i < 2 * ORDEM) {
      printf("chave: %d ", root->index[i]);
      i++;
    }
  } else {
    Page p;
    get_page(fp, &p, page);
    i = 0;
    while (p.records[i] != -1 && i < REGS_POR_PAGINA) {
      printf("chave: %d ", get_key_from_rfile(fr, p.records[i]));
      i++;
    }
  }
  printf("\n");
}

//================================== B+ Tree ===========================
Page* new_page() {
  Page* page = (Page*) malloc(sizeof(Page));
  int i;
  for (i = 0; i < REGS_POR_PAGINA; i++) {
    page->records[i] = -1;
  }
  page->fnum = -1;
  page->next = -1;
  page->stored = 0;
  return page;
}

void logical_remove_record(Page* p, int i) {
  p->records[i] = -1;
  p->stored--;
}

bool tmp_insert_record_in_page(FILE* fp, FILE* fr, int r_num, Page* page) {
  Record r;
  get_record(fr, &r, r_num);
  int j;
  for (j = 0;
      page->records[j] != -1 && page->stored < REGS_POR_PAGINA
      && r.key > get_key_from_rfile(fr, page->records[j]); j++)
    ;
  if (page->records[j] != -1
      && r.key == get_key_from_rfile(fr, page->records[j]))
    return false;
  int p;
  int next;
  int saved = -1;
  next = page->records[j];
  for (p = j; page->records[p] != -1 && p < REGS_POR_PAGINA - 1; p++) {
    saved = page->records[p + 1];
    page->records[p + 1] = next;
    next = saved;
  }
  page->records[j] = r.rnum;
  page->stored++;
  return true;
}

int get_key_from_rfile(FILE* fr, int offset) {
  Record r;
  get_record(fr, &r, offset);
  return r.key;
}

bool insert_record_in_page_noparent(FILE* fp, FILE* fr, int r_num, Page* page,
    InternalNode* root, int r_index) {
  Record r;
  get_record(fr, &r, r_num);
  int j;
  for (j = 0;
      page->records[j] != -1 && page->stored < REGS_POR_PAGINA
      && r.key > get_key_from_rfile(fr, page->records[j]); j++)
    ;
  if (page->records[j] != -1
      && r.key == get_key_from_rfile(fr, page->records[j]))
    return false;

  int p;
  int next;
  int saved = -1;
  next = page->records[j];
  for (p = j; page->records[p] != -1 && p < REGS_POR_PAGINA - 1; p++) {
    saved = page->records[p + 1];
    page->records[p + 1] = next;
    next = saved;
  }
  page->records[j] = r.rnum;
  page->stored++;
  write_page_on_file_usefnum(fp, page);
  return true;
}

bool insert_record_in_page(FILE* fp, FILE* fr, int r_num, Page* page,
    InternalNode* root, int r_index) {
  Record r;
  get_record(fr, &r, r_num);

  int j;
  for (j = 0;
      page->records[j] != -1 && page->stored < REGS_POR_PAGINA
      && r.key > get_key_from_rfile(fr, page->records[j]); j++)
    ;
  if (page->records[j] != -1
      && r.key == get_key_from_rfile(fr, page->records[j]))
    return false;

  int p;
  int next;
  int saved = -1;
  next = page->records[j];
  for (p = j; page->records[p] != -1 && p < REGS_POR_PAGINA - 1; p++) {
    saved = page->records[p + 1];
    page->records[p + 1] = next;
    next = saved;
  }
  page->records[j] = r.rnum;
  page->stored++;
  write_page_on_file(fp, page, &root->pages[r_index]);
  return true;
}
///*
// * 1 - b is before the middle
// * 2 - b is in the middle
// * 3 - b is after the middle
// */
void splitting_pages(FILE* fp, FILE* fr, Page *page, Record* r, int state,
    InternalNode* parent) {

  Page* p2 = new_page();
  int middleIndex = (REGS_POR_PAGINA - 1) / 2; //middle element when the node is full
  int f;
  int up_key;
  switch (state) {
    case 1:
      //		printf("B está antes do elemento do meio\n");
      up_key = get_key_from_rfile(fr, page->records[middleIndex]);
      //		printf("Meio é o %d\n",(get_key_from_rfile(fr,page->records[middleIndex])));
      for (f = middleIndex + 1; f < REGS_POR_PAGINA; f++) {
        tmp_insert_record_in_page(fp, fr, page->records[f], p2);
        logical_remove_record(page, f); //remove the record in index f
      }
      if (page->next != -1)
        p2->next = page->next;
      write_page_on_file_new(fp, p2);
      page->next = p2->fnum; //will be save in file by insert_record_in_page
      //NÃO TEM REMOÇÃO LÓGICA DO MEIO, É UMA ÁRVORE B+, TEM CÓPIA
      tmp_insert_record_in_page(fp, fr, r->rnum, page);
      write_page_on_file_usefnum(fp, page);

      insert_at_index_with_page(parent, up_key, true, p2->fnum);
      break;
    case 2:
      //		printf("%d é o elemento do meio\n",r->key);
      up_key = r->key;
      for (f = middleIndex + 1; f < REGS_POR_PAGINA; f++) {
        tmp_insert_record_in_page(fp, fr, page->records[f], p2);
        logical_remove_record(page, f); //remove the record in index f
      }
      if (page->next != -1)
        p2->next = page->next;
      write_page_on_file_new(fp, p2);
      page->next = p2->fnum; //will be save in file by insert_record_in_page
      tmp_insert_record_in_page(fp, fr, r->rnum, page);
      write_page_on_file_usefnum(fp, page);
      insert_at_index_with_page(parent, up_key, true, p2->fnum);
      break;
    case 3:
      up_key = get_key_from_rfile(fr, page->records[++middleIndex]);
      //		printf("%d é o elemento do meio\n",up_key);
      for (f = middleIndex + 1; f < REGS_POR_PAGINA; f++) {
        tmp_insert_record_in_page(fp, fr, page->records[f], p2);
        logical_remove_record(page, f); //remove the record in index f
      }
      tmp_insert_record_in_page(fp, fr, r->rnum, p2);
      if (page->next != -1)
        p2->next = page->next;
      write_page_on_file_new(fp, p2);
      page->next = p2->fnum; //will be save in file by insert_record_in_page
      //NÃO TEM REMOÇÃO LÓGICA DO MEIO, É UMA ÁRVORE B+, TEM CÓPIA
      write_page_on_file_usefnum(fp, page);
      insert_at_index_with_page(parent, up_key, true, p2->fnum);
      break;
  }
}

void write_page_on_file_usefnum(FILE* f, Page* p) {
  int e;
  e = fseek(f, p->fnum * sizeof(Page), SEEK_SET);
  if (e == 0) {
    fwrite(p, sizeof(Page), 1, f);
  }
}

void write_page_on_file_new(FILE* f, Page* p) {
  int e;
  e = fseek(f, TOTAL_PAGES_FILE * sizeof(Page), SEEK_SET);
  if (e == 0) {
    p->fnum = TOTAL_PAGES_FILE;
    TOTAL_PAGES_FILE++;
    fwrite(p, sizeof(Page), 1, f);
  }
}

void write_page_on_file(FILE* f, Page* p, int* parent_pointer) {
  int e;
  if ((*parent_pointer) == -1) {
    e = fseek(f, TOTAL_PAGES_FILE * sizeof(Page), SEEK_SET);
  } else {
    e = fseek(f, (*parent_pointer) * sizeof(Page), SEEK_SET);
  }
  if (e == 0) {
    if ((*parent_pointer) == -1) {
      p->fnum = TOTAL_PAGES_FILE;
      *parent_pointer = TOTAL_PAGES_FILE;
      TOTAL_PAGES_FILE++;
    }
    fwrite(p, sizeof(Page), 1, f);

  }
}

void get_record(FILE *f, Record *r, int offset) {
  fseek(f, offset * sizeof(Record), SEEK_SET);
  fread(r, sizeof(Record), 1, f);
}

void get_page(FILE *f, Page *r, int offset) {
  fseek(f, offset * sizeof(Page), SEEK_SET);
  fread(r, sizeof(Page), 1, f);
}

bool analyze_split(FILE* fp, FILE* fr, Record* r, Page* page,
    InternalNode* root, int root_index) {
  int middle = (REGS_POR_PAGINA - 1) / 2;
  if (r->key > get_key_from_rfile(fr, page->records[middle])) {
    if (root_index != -1)
      logicalRemove(root, root_index);
    if (r->key > get_key_from_rfile(fr, page->records[middle + 1]))
      splitting_pages(fp, fr, page, r, 3, root); //r is after the middle
    else
      splitting_pages(fp, fr, page, r, 2, root); //r is the middle
  } else if (r->key < get_key_from_rfile(fr, page->records[middle])) {
    if (root_index != -1)
      logicalRemove(root, root_index);
    splitting_pages(fp, fr, page, r, 1, root); //r is before the middle
  } else
    return false; //if(node->records[t] == r->key)
  return true;
}

bool insert_record(FILE* fp, FILE* fr, InternalNode* root, Record* r) {
  if (root->leaf == true) {
    int i = 0;
    for (i = 0;
        i < 2 * ORDEM && root->index[i] != -1 && r->key > root->index[i];
        i++)
      ;

    if (i == 2 * ORDEM) {
      if (root->pages[i] == -1) {
        //=======================================
        Page p;
        get_page(fp, &p, root->pages[i - 1]);
        if (p.stored < REGS_POR_PAGINA) {
          changeValue(root, i - 1, r->key);
          insert_record_in_page_noparent(fp, fr, r->rnum, &p, root,
              i);
        } else {
          return analyze_split(fp, fr, r, &p, root, i - 1);
        }
        //=======================================
      } else {
        //=======================================
        Page p;
        get_page(fp, &p, root->pages[i]);
        if (p.stored < REGS_POR_PAGINA) {
          insert_record_in_page_noparent(fp, fr, r->rnum, &p, root,
              i);
        } else {
          return analyze_split(fp, fr, r, &p, root, -1);
        }
        //=======================================
      }
    } else if (r->key <= root->index[i]) { //nao é nulo, existe chave, então existe página
      Page p;
      get_page(fp, &p, root->pages[i]);
      if (p.stored < REGS_POR_PAGINA) {
        insert_record_in_page_noparent(fp, fr, r->rnum, &p, root, i);
      } else {
        if (i < 2 * ORDEM && root->pages[i + 1] == -1) {
          return analyze_split(fp, fr, r, &p, root, i);
        } else {
          return analyze_split(fp, fr, r, &p, root, -1);
        }
      }
    } else { //index == -1 || k->r < root->records[i]
      if (root->pages[i] == -1) {
        //=======================================
        //				CRIA NOVA PÁGINA
        //				Page* p = new_page();
        //				p->records[0] = r;
        //				write_page_on_file(fp,p,&root->pages[i]);
        Page p;
        get_page(fp, &p, root->pages[i - 1]);
        if (p.stored < REGS_POR_PAGINA) {
          changeValue(root, i - 1, r->key);
          insert_record_in_page_noparent(fp, fr, r->rnum, &p, root,
              i);
        } else {
          analyze_split(fp, fr, r, &p, root, i - 1);
        }
        //=======================================
      } else {
        //=======================================
        Page p;
        get_page(fp, &p, root->pages[i]);
        if (p.stored < REGS_POR_PAGINA) {
          insert_record_in_page_noparent(fp, fr, r->rnum, &p, root,
              i);
        } else { //index already is -1
          analyze_split(fp, fr, r, &p, root, -1);
        }
        //=======================================
      }
    }
  } else { //INTERNAL NODE
    int i = 0;
    if (r->key <= root->index[i]) {
      return insert_record(fp, fr, root->pointers[i], r); //validation is on this method if pointers[i] is NULL
    } else { //>
      while (root->index[i] != -1 && i < 2 * ORDEM) {
        if (r->key <= root->index[i])
          return insert_record(fp, fr, root->pointers[i], r);
        i++; //key is greater than the index[i]
      }
      return insert_record(fp, fr, root->pointers[i], r); //full or -1
    }
  }
  return true;
}
//=================================== B Tree =========================

//================================= REMOCAO  ===============================
InternalNode* nextInOrderByKeyNode(int b) {
  InternalNode *node = searchKey(HEAD, b);
  int i = keyIndexFromNode(node, b);
  InternalNode* p = node->pointers[i + 1];
  while (p->pointers[0] != NULL)
    p = p->pointers[0];
  return p;
}

int nextInOrderByKey(int b) {
  InternalNode* node = nextInOrderByKeyNode(b);
  return node->index[0];
}

int keyIndexFromNode(InternalNode* node, int key) {
  int i;
  for (i = 0; node->index[i] != key; i++)
    ;
  return i;
}

int nodePageIndexFromNode(InternalNode* parent, Page* pChild) {
  int i;
  for (i = 0; parent->pages[i] != pChild->fnum; i++)
    ;
  return i;
}

int nodePointerIndexFromNode(InternalNode* parent, InternalNode* child) {
  int i;
  for (i = 0; parent->pointers[i] != child; i++)
    ;
  return i;
}

void delete_key_leaf(InternalNode* root, int iIndex) {
  root->stored--;
  int i;
  int j = iIndex;
  if (iIndex < 2 * ORDEM && root->pages[iIndex + 1] == -1)
    j++;
  for (i = iIndex; i + 1 < 2 * ORDEM && root->index[i + 1] != -1; i++, j++) {
    root->index[i] = root->index[i + 1];
    root->pages[j] = root->pages[j + 1];
  }
  root->index[i] = -1;
  root->pages[j] = -1;
}

void delete_key(InternalNode* root, int iIndex) {
  root->stored--;
  int i;
  int j = iIndex;
  if (iIndex < 2 * ORDEM && root->pointers[iIndex + 1] == NULL)
    j++;
  for (i = iIndex; i + 1 < 2 * ORDEM && root->index[i + 1] != -1; i++, j++) {
    root->index[i] = root->index[i + 1];
    root->pointers[j] = root->pointers[j + 1];
  }
  root->index[i] = -1;
  root->pointers[j] = NULL;
}

int get_index_from_last_element_page(Page* page) {
  int i;
  for (i = 0; i < REGS_POR_PAGINA && page->records[i] != -1; i++)
    ;
  return --i;
}

int get_index_from_last_element(InternalNode* node) {
  int i;
  for (i = 0; i < 2 * ORDEM && node->index[i] != -1; i++)
    ;
  return --i;
}

bool remove_action_leaf(FILE* fp,FILE* fr,InternalNode*node, int i) {
  if (node->stored == 0 && node->pages[0] != -1) {
    Page p;
    get_page(fp,&p,node->pages[0]);
    int t = get_index_from_last_element_page(&p);
    node->index[i] = get_key_from_rfile(fr,p.records[t]);
    return true;
  }
  //the boolean UP will be necessary when we deal with records
  if (node->stored < ORDEM) {
    InternalNode* sibling_node;
    //siblings
    if (node->parent) { //if it isn't the root
      int f = nodePointerIndexFromNode(node->parent, node);
      if (f < 2 * ORDEM && node->parent->pointers[f + 1] != NULL) {
        sibling_node = node->parent->pointers[f + 1];
        if (sibling_node->stored > ORDEM) {
          //REDISTRIBUTING == THE INSERTION WILL BE IN THE NODE!
          //RIGHT SIBLING HELP (REDISTRIBUTING) ----------OK-------------
          int p = nodePointerIndexFromNode(node->parent, node);
          int pKey = node->parent->index[p];

          insert_at_index_with_page(node, pKey,
              true/* doesn't care, it is a leaf*/,
              sibling_node->pages[0]);
          insert_in_this_position(node->parent,
              sibling_node->index[0], f, NULL);

          delete_key_leaf(sibling_node, 0);
          //------------------------------------------------------
          return true;
        } else if (f > 0 && node->parent->pointers[f - 1] != NULL) {
          //there is a left sibling
          sibling_node = node->parent->pointers[f - 1];
          if (sibling_node->stored > ORDEM) {
            //LEFT SIBLING HELP (REDISTRIBUTING)----------OK----------
            int pKey = node->parent->index[f - 1];
            int t = get_index_from_last_element(sibling_node);
            node->pages[0] = sibling_node->pages[t + 1];
            sibling_node->pages[t + 1] = -1; //change pointers
            int go_up = sibling_node->index[t];
            insert(node, pKey, true/* doesn't care, it is a leaf*/,
                NULL);
            insert_in_this_position(node->parent, go_up, f - 1,
                NULL);
            delete_key_leaf(sibling_node, t);
            //--------------------------------------------------
            return true;
          } else {
            //MERGE WITH RIGHT SIBLING--------------------------
            int pKey = node->parent->index[f];
            insert_at_index_with_page(node, pKey,
                true/* doesn't care, it is a leaf*/,
                sibling_node->pages[0]);
            sibling_node->pages[0] = -1;

            int j;
            for (j = 0; sibling_node->index[j] != -1; j++) {
              insert_at_index_with_page(node, sibling_node->index[j],
                  true, sibling_node->pages[j + 1]);
              //						delete_key_leaf(sibling_node, j); //isn't necessary
            }
            node->parent->pointers[f + 1] = NULL;

            delete_key_leaf(node->parent, f);
            remove_action(node->parent, f);
            //--------------------------------------------------
            return true;
          }
        } else {
          //MERGE WITH RIGHT SIBLING--------------------------
          int pKey = node->parent->index[f];
          insert_at_index_with_page(node, pKey,
              true/* doesn't care, it is a leaf*/,
              sibling_node->pages[0]);
          sibling_node->pages[0] = -1;

          int j;
          for (j = 0; sibling_node->index[j] != -1; j++) {
            insert_at_index_with_page(node, sibling_node->index[j],
                true, sibling_node->pages[j + 1]);
            //						delete_key_leaf(sibling_node, j); //isn't necessary
          }
          node->parent->pointers[f + 1] = NULL;

          delete_key_leaf(node->parent, f);
          remove_action(node->parent, f);
          //--------------------------------------------------
        }
      } else if (f > 0 && node->parent->pointers[f - 1] != NULL) {
        sibling_node = node->parent->pointers[f - 1];
        if (sibling_node->stored > ORDEM) {
          //LEFT SIBLING HELP (REDISTRIBUTING) ---------OK---------------
          int pKey = node->parent->index[f - 1];
          int t = get_index_from_last_element(sibling_node);
          node->pages[0] = sibling_node->pages[t + 1];
          sibling_node->pages[t + 1] = -1; //change pointers
          int go_up = sibling_node->index[t];
          insert(node, pKey, true/* doesn't care, it is a leaf*/,
              NULL);
          insert_in_this_position(node->parent, go_up, f - 1, NULL);
          delete_key_leaf(sibling_node, t);
          //------------------------------------------------------
          return true;
        } else {
          //MERGE WITH LEFT SIBLING-------------------------------
          int pKey = node->parent->index[f - 1];
          insert_at_index_with_page(sibling_node, pKey,
              true/* doesn't care, it is a leaf*/,
              node->pages[0]);
          node->pages[0] = -1;

          int j;
          for (j = 0; node->index[j] != -1; j++) {
            insert_at_index_with_page(sibling_node, node->index[j],
                true, node->pages[j + 1]);
            node->pages[j + 1] = -1;
            //	delete_key_leaf(node, j); //isn't necessary
          }
          node->parent->pointers[f] = NULL;

          delete_key_leaf(node->parent, f - 1);
          remove_action(node->parent, f - 1);
          //------------------------------------------------------
          return true;
        }
      }
    }
  }
  return true;
}

bool remove_action(InternalNode* node, int i/*key in index array*/) {
  if (node->stored == 0 && node->pointers[0] != NULL) {
    HEAD = node->pointers[0];
    node->pointers[0]->parent = NULL;
    return true;
  }
  //the boolean UP will be necessary when we deal with records
  if (node->stored < ORDEM) {
    InternalNode* sibling_node;
    //siblings
    if (node->parent) { //if it isn't the root
      int f = nodePointerIndexFromNode(node->parent, node);
      if (f < 2 * ORDEM && node->parent->pointers[f + 1] != NULL) {
        sibling_node = node->parent->pointers[f + 1];
        if (sibling_node->stored > ORDEM) {
          //REDISTRIBUTING == THE INSERTION WILL BE IN THE NODE!
          //RIGHT SIBLING HELP (REDISTRIBUTING) ----------OK-------------
          int p = nodePointerIndexFromNode(node->parent, node);
          int pKey = node->parent->index[p];
          insert(node, pKey, true/* doesn't care, it is a leaf*/,
              sibling_node->pointers[0]);
          insert_in_this_position(node->parent,
              sibling_node->index[0], f, NULL);
          delete_key(sibling_node, 0);
          //------------------------------------------------------
          return true;
        } else if (f > 0 && node->parent->pointers[f - 1] != NULL) {
          //there is a left sibling
          sibling_node = node->parent->pointers[f - 1];
          if (sibling_node->stored > ORDEM) {
            //LEFT SIBLING HELP (REDISTRIBUTING)----------OK----------
            int pKey = node->parent->index[f - 1];
            int t = get_index_from_last_element(sibling_node);
            node->pointers[0] = sibling_node->pointers[t + 1];
            sibling_node->pointers[t + 1] = NULL; //change pointers
            int go_up = sibling_node->index[t];
            insert(node, pKey, true/* doesn't care, it is a leaf*/,
                NULL);
            insert_in_this_position(node->parent, go_up, f - 1,
                NULL);
            delete_key(sibling_node, t);
            //--------------------------------------------------
            return true;
          } else {
            //MERGE WITH RIGHT SIBLING--------------------------
            int pKey = node->parent->index[f];
            insert(node, pKey, true/* doesn't care, it is a leaf*/,
                sibling_node->pointers[0]);
            sibling_node->pointers[0] = NULL;

            int j;
            for (j = 0; sibling_node->index[j] != -1; j++) {
              insert(node, sibling_node->index[j], true,
                  sibling_node->pointers[j + 1]);
              //delete_key(sibling_node, j); //isn't necessary
            }
            node->parent->pointers[f + 1] = NULL;

            delete_key(node->parent, f);
            remove_action(node->parent, f);
            //--------------------------------------------------
            return true;
          }
        } else {
          //MERGE WITH RIGHT SIBLING--------------------------
          int pKey = node->parent->index[f];
          insert(node, pKey, true/* doesn't care, it is a leaf*/,
              sibling_node->pointers[0]);
          sibling_node->pointers[0] = NULL;

          int j;
          for (j = 0; sibling_node->index[j] != -1; j++) {
            insert(node, sibling_node->index[j], true,
                sibling_node->pointers[j + 1]);
            //delete_key(sibling_node, j); //isn't necessary
          }
          node->parent->pointers[f + 1] = NULL;

          delete_key(node->parent, f);
          remove_action(node->parent, f);
          //--------------------------------------------------
        }
      } else if (f > 0 && node->parent->pointers[f - 1] != NULL) {
        sibling_node = node->parent->pointers[f - 1];
        if (sibling_node->stored > ORDEM) {
          //LEFT SIBLING HELP (REDISTRIBUTING) ---------OK---------------
          int pKey = node->parent->index[f - 1];
          int t = get_index_from_last_element(sibling_node);
          node->pointers[0] = sibling_node->pointers[t + 1];
          sibling_node->pointers[t + 1] = NULL; //change pointers
          int go_up = sibling_node->index[t];
          insert(node, pKey, true/* doesn't care, it is a leaf*/,
              NULL);
          insert_in_this_position(node->parent, go_up, f - 1, NULL);
          delete_key(sibling_node, t);
          //------------------------------------------------------
          return true;
        } else {
          //MERGE WITH LEFT SIBLING-------------------------------
          int pKey = node->parent->index[f - 1];
          insert(sibling_node, pKey,
              true/* doesn't care, it is a leaf*/,
              node->pointers[0]);
          node->pointers[0] = NULL;

          int j;
          for (j = 0; node->index[j] != -1; j++) {
            insert(sibling_node, node->index[j], true,
                node->pointers[j + 1]);
            //	delete_key(node, j); //isn't necessary
          }
          node->parent->pointers[f] = NULL;

          delete_key(node->parent, f - 1);
          remove_action(node->parent, f - 1);
          //------------------------------------------------------
          return true;
        }
      }
    }
  }
  return true;
}

bool remove_action_page(FILE* fp, FILE* fr, Page* node, InternalNode* parent,
    int i/*key in index array*/) {

  if (node->stored < REGS_POR_PAGINA / 2) {
    Page sibling_node;
    if (i < 2 * ORDEM && parent->pages[i + 1] != -1) { //there is a right sibling
      get_page(fp, &sibling_node, parent->pages[i + 1]);
      if (sibling_node.stored > REGS_POR_PAGINA / 2) {
        //REDISTRIBUTING == THE INSERTION WILL BE IN THE NODE!
        //RIGHT SIBLING HELP (REDISTRIBUTING) ----------OK-------------
        //THE SAME SIBLING NODE WILL BE ADD IN PARENT AND PAGE NODE (b+tree)
        Record r;
        get_record(fr, &r, sibling_node.records[0]);
        insert_record_in_page_noparent(fp, fr, r.rnum, node, parent, i);
        //change Value
        changeValue(parent, i, r.key);
        delete_key_page(fp, &sibling_node, 0);
        //------------------------------------------------------
        return true;
      } else if (i > 0 && parent->pages[i - 1] != -1) { //there is a left sibling
        //there is a left sibling
        get_page(fp, &sibling_node, parent->pages[i - 1]);
        if (sibling_node.stored > REGS_POR_PAGINA / 2) {
          //LEFT SIBLING HELP (REDISTRIBUTING)----------OK----------
          int t = get_index_from_last_element_page(&sibling_node);
          Record r;
          get_record(fr, &r, sibling_node.records[t]);
          insert_record_in_page_noparent(fp, fr, r.rnum, node, parent,
              i);
          delete_key_page(fp, &sibling_node, t);
          //novo ultimo, antigo antepenultimo ira subir
          t = get_index_from_last_element_page(&sibling_node);
          int go_up = get_key_from_rfile(fr, sibling_node.records[t]);
          ;
          //change Value
          changeValue(parent, i - 1, go_up);
          //--------------------------------------------------
          return true;
        } else {
          get_page(fp, &sibling_node, parent->pages[i + 1]);
          //MERGE WITH RIGHT SIBLING--------------------------
          int j;
          for (j = 0; sibling_node.records[j] != -1; j++) {
            insert_record_in_page_noparent(fp, fr,
                sibling_node.records[j], node, parent, i);
            //						delete_key_page(fp, &sibling_node, j);
          }
          receive_next_and_save(fp, node, sibling_node.next);
          parent->pages[i + 1] = -1;
          delete_key_leaf(parent, i);
          remove_action_leaf(fp,fr,parent, i);
          //--------------------------------------------------
          return true;
        }
      } else {
        //MERGE WITH RIGHT SIBLING--------------------------
        int j;
        for (j = 0; sibling_node.records[j] != -1; j++) {
          insert_record_in_page_noparent(fp, fr,
              sibling_node.records[j], node, parent, i);
          //					delete_key_page(fp, &sibling_node, j);
        }
        receive_next_and_save(fp, node, sibling_node.next);
        parent->pages[i + 1] = -1;
        delete_key_leaf(parent, i);
        remove_action_leaf(fp,fr,parent, i);
        //--------------------------------------------------
      }
    } else //there is only left sibling
      if (i > 0 && parent->pages[i - 1] != -1) {
        get_page(fp, &sibling_node, parent->pages[i - 1]);
        if (sibling_node.stored > REGS_POR_PAGINA / 2) {
          //LEFT SIBLING HELP (REDISTRIBUTING)----------OK----------
          int t = get_index_from_last_element_page(&sibling_node);
          Record r;
          get_record(fr, &r, sibling_node.records[t]);
          insert_record_in_page_noparent(fp, fr, r.rnum, node, parent, i);
          delete_key_page(fp, &sibling_node, t);
          //novo ultimo, antigo antepenultimo ira subir
          t = get_index_from_last_element_page(&sibling_node);
          int go_up = get_key_from_rfile(fr, sibling_node.records[t]);
          ;
          //change Value
          changeValue(parent, i - 1, go_up);
          //--------------------------------------------------
          return true;
        } else {
          //MERGE WITH LEFT SIBLING-------------------------------
          int j;
          for (j=0; node->records[j] != -1 && j < REGS_POR_PAGINA;j++) {
            insert_record_in_page_noparent(fp, fr, node->records[j],
                &sibling_node, parent, i);
            //					delete_key_page(fp, &node, j);
          }
          receive_next_and_save(fp, &sibling_node, node->next);
          parent->pages[i] = -1;
          delete_key_leaf(parent, i - 1);
          remove_action_leaf(fp,fr,parent, i - 1);
          //------------------------------------------------------
          return true;
        }
      } else {
        if (node->stored == 0) {
          HEAD = NULL;
          return true;
        }
      }
  } //at least half full, do nothing
  return true;
}

void receive_next_and_save(FILE* fp, Page* receiver, int next) {
  receiver->next = next;
  write_page_on_file_usefnum(fp, receiver);
}

bool removing(int b) {
  InternalNode *node = searchKey(HEAD, b);
  if (node == NULL)
    return false;
  int i = keyIndexFromNode(node, b);
  if (!node->leaf) {
    int t = nextInOrderByKey(b);
    InternalNode* subs = searchKey(HEAD, t);
    node->index[i] = t;
    i = keyIndexFromNode(subs, t);
    delete_key(subs, i);
    return remove_action(subs, i);
  } else {
    delete_key(node, i);
    return remove_action(node, i);
  }
}

void delete_key_page(FILE* fp, Page* page, int iIndex) {
  int i;
  for (i = iIndex; i + 1 < REGS_POR_PAGINA && page->records[i + 1] != -1;
      i++) {
    page->records[i] = page->records[i + 1];
  }
  page->records[i] = -1;
  page->stored--;
  write_page_on_file_usefnum(fp, page);
}

bool removing_page(FILE* fp, FILE* fr, int b) {
  int i = 0;
  InternalNode* current = searchKeyBPlus(fp, fr, HEAD, b, &i);
  if (current == NULL)
    return false;
  Page p;
  get_page(fp, &p, current->pages[i]);
  int r = search_at_record(fp, fr, &p, b);
  if (r == -1)
    return false;
  delete_key_page(fp, &p, r);
  remove_action_page(fp, fr, &p, current, i);
  return true;
}

void remove_key(FILE* fp, FILE* fr) {
  int b;
  scanf("%d%*c", &b);
  if (b != -1) {
    if (!removing_page(fp, fr, b))
      printf("chave nao encontrada: %d\n", b);
  }
  return;
}

//=========================== BUSCA DE REGISTRO==============================

InternalNode* searchKeyBPlus(FILE* fp, FILE* fr, InternalNode* root, int key,
    int* i) {
  if (root == NULL)
    return NULL;
  /*
   * carrega página, vê se tem espaço, adiciona se tiver,
   * verifica posição e faz slipt
   *
   */
  if (root->leaf == true) {
    (*i) = 0;
    if (key <= root->index[*i]) { //nao é nulo, existe chave, então existe página
      return root;
    } else { //>
      while (root->index[*i] != -1 && (*i) < 2 * ORDEM) {
        if (key <= root->index[*i]) {
          return root;
        }
        (*i)++; //key is greater than the index[i]
      }
      if (root->pages[*i] == -1) { //nao existente ainda
        return NULL;
      } else {
        return root; //full or -1
      }
    }

  } else { //INTERNAL NODE
    (*i) = 0;
    if (key <= root->index[*i]) {
      return searchKeyBPlus(fp, fr, root->pointers[*i], key, i);
      //validation is on this method if pointers[i] is NULL
    } else { //>
      while (root->index[*i] != -1 && (*i) < 2 * ORDEM) {
        if (key <= root->index[*i]) {

          return searchKeyBPlus(fp, fr, root->pointers[*i], key, i);

        }
        (*i)++; //key is greater than the index[i]
      }

      return searchKeyBPlus(fp, fr, root->pointers[*i], key, i); //full or -1
    }
  }
  return NULL;
}
int search_at_record(FILE* fp, FILE* fr, Page* p, int key) {
  int j;
  for (j = 0; j < REGS_POR_PAGINA && p->records[j] != -1; j++) {
    Record r;
    get_record(fr, &r, p->records[j]);
    if (r.key == key) {
      return j;
    }
  }
  return -1;
}

InternalNode* searchKey(InternalNode* root, int key) {
  if (root == NULL)
    return NULL;
  int i = 0;
  if (root->index[i] == -1)
    return NULL; //key was not found
  else if (root->index[i] == key) {
    return root;
  } else if (key < root->index[i]) {
    return searchKey(root->pointers[i], key); //validation is on this method if pointers[i] is NULL
  } else { //>
    while (root->index[i] != -1 && i < 2 * ORDEM) {
      if (key < root->index[i])
        return searchKey(root->pointers[i], key);
      else if (key == root->index[i])
        return root;
      i++; //key is greater than the index[i]
    }
    if (root->index[i] == -1)
      return searchKey(root->pointers[i], key);
    else
      return searchKey(root->pointers[i], key); //this i represent i+1 cause the array was full
  }
}

void search_BPlustree(FILE* fp, FILE* fr, InternalNode* root) {
  int b;
  scanf("%d%*c", &b);
  if (b != -1) {
    int i = 0;
    InternalNode* current = searchKeyBPlus(fp, fr, root, b, &i);
    int r = -1;
    Page p;
    if (current != NULL) {
      get_page(fp, &p, current->pages[i]);
      r = search_at_record(fp, fr, &p, b);
    }
    if (r == -1)
      printf("chave nao encontrada: %d\n", b);
    else {
      Record record;
      get_record(fr, &record, p.records[r]);
      printf("chave: %d\n", record.key);
      printf("%s\n", record.name);
      printf("%d\n", record.age);
    }
  }
  return;
}

void search_Btree(InternalNode* root) {
  int b;
  scanf("%d%*c", &b);
  if (b != -1) {
    InternalNode* r = searchKey(root, b);
    if (r == NULL)
      printf("chave nao encontrada: %d\n", b);
    else {
      printf("chave: %d\n", b);
    }
  }
  return;
}

//========================== READ ALL PAGES ======================

void show_page(FILE* fp, FILE* fr, int iPage, int* t) {
  if (iPage == -1)
    return;
  Page page;
  get_page(fp, &page, iPage);
  printf("No: %d\n", (*t)++);
  int i;
  for (i = 0; i < REGS_POR_PAGINA && page.records[i] != -1; i++) {
    Record r;
    get_record(fr, &r, page.records[i]);
    printf("%d\n", r.key);
    printf("%s\n", r.name); //name value
    printf("%d\n", r.age); //age value
  }
  if (page.next != -1)
    show_page(fp, fr, page.next, t);

}

void final_nodes(FILE* fp, FILE* fr, InternalNode* root) {
  if (root == NULL)
    return;
  while (!root->leaf) {
    root = root->pointers[0];
  }
  int total = 1;
  if (root->pages[0] != -1) {
    show_page(fp, fr, root->pages[0], &total);
  }
}

//========================= INSERCAO DE REGISTROS ===============================

void btree2(FILE* fp, FILE* fr, InternalNode* root) {
  int key, age, i = 0;
  char name[MAX_NAME];
  scanf("%d%*c", &key);
  InternalNode* current = searchKeyBPlus(fp, fr, root, key, &i);
  int r = -1;
  if (current != NULL) {
    Page p;
    get_page(fp, &p, current->pages[i]);
    r = search_at_record(fp, fr, &p, key);
  }
  if (r != -1){
    char name[MAX_NAME];
    int age;
    scanf("%[^\n]s%*c",name);
    scanf("%d%*c", &age);
    printf("chave ja existente: %d\n", key);
  }
  else {
    Record* r = new_record(fr, key);
    if (root == NULL) {
      root = new_node(NULL);
      root->leaf = true;
      HEAD = root;
      Page* p = new_page();
      p->records[0] = r->rnum;
      p->stored++;
      write_page_on_file(fp, p, &root->pages[0]);
      insert(root, key, true, NULL);
      return;
    }
    insert_record(fp, fr, root, r);
  }
  return;
}

void insert_in_this_position_page(InternalNode* root, int b, int iIndex,
    int child) {
  root->stored++;
  root->index[iIndex] = b;
  if (child != -1) {
    root->pages[iIndex + 1] = child;
  }
}

void insert_in_this_position(InternalNode* root, int b, int iIndex,
    InternalNode* child) {
  root->stored++;
  root->index[iIndex] = b;
  if (child != NULL) {
    root->pointers[iIndex + 1] = child;
    child->parent = root;
  }
}

void passPointer(InternalNode* receiver, InternalNode* sender, int f) {
  receiver->pointers[f] = sender->pointers[f];
  sender->pointers[f] = NULL;
}

void getLastPointerFromSibling_page(InternalNode* receiver,
    InternalNode* sender) {
  int i;
  for (i = 0; sender->index[i] != -1 && i < 2 * ORDEM; i++)
    ;
  receiver->pages[0] = sender->pages[i + 1];
  sender->pages[i + 1] = -1;
}

void getLastPointerFromSibling(InternalNode* receiver, InternalNode* sender) {
  int i;
  for (i = 0; sender->index[i] != -1 && i < 2 * ORDEM; i++)
    ;
  receiver->pointers[0] = sender->pointers[i + 1];
  if (receiver->pointers[0] != NULL) { //nao estava vazio
    receiver->pointers[0]->parent = receiver;
    sender->pointers[i + 1] = NULL;
  }
}
//==========================================================================================

/*
 * 1 - b is before the middle
 * 2 - b is in the middle
 * 3 - b is after the middle
 */
void splitting2(InternalNode *root, int b, int state, InternalNode* parent,
    int CHILD) {
  InternalNode* p2 = new_node(root);
  int middleIndex = ORDEM - 1; //middle element when the node is full
  int tmp;
  int f;
  p2->leaf = true;
  switch (state) {
    case 1:
      //		printf("B está antes do elemento do meio\n");
      //		printf("Meio é o %d\n",root->index[middleIndex]);
      for (f = ORDEM; f < 2 * ORDEM; f++) {
        insert_at_index_with_page(p2, root->index[f], true,
            root->pages[f + 1]);
        root->pages[f + 1] = -1;
        logicalRemove(root, f);
      }
      tmp = root->index[middleIndex];
      logicalRemove(root, middleIndex);
      if (parent) {
        p2->parent = parent;
        insert(parent, tmp, true, p2);
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert(new_root, tmp, true, NULL);
      }
      //--- exatamente nesta ordem
      getLastPointerFromSibling_page(p2, root);
      insert_at_index_with_page(root, b, true, CHILD);
      break;
    case 2:
      //		printf("B é o elemento do meio\n");
      middleIndex++;
      for (f = ORDEM; f < 2 * ORDEM; f++) {
        insert_at_index_with_page(p2, root->index[f], true,
            root->pages[f + 1]);
        root->pages[f + 1] = -1;
        logicalRemove(root, f);
      }
      if (parent) {
        p2->parent = parent;
        //p2->pointer[0] = child;//rouba do parametro
        insert(parent, b, true, p2);
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert(new_root, b, true, NULL);
      }
      p2->pages[0] = CHILD;
      break;
    case 3:
      //		printf("B esta depois do elemento do meio\n");
      for (f = ORDEM + 1; f < 2 * ORDEM; f++) {
        insert_at_index_with_page(p2, root->index[f], true,
            root->pages[f + 1]);
        root->pages[f + 1] = -1;
        logicalRemove(root, f);
      }
      tmp = root->index[++middleIndex];
      logicalRemove(root, middleIndex);
      if (parent) {
        p2->parent = parent;
        insert(parent, tmp, true, p2);
        insert_at_index_with_page(p2, b, true, CHILD); // add child
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        insert(new_root, tmp, true, NULL);
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert_at_index_with_page(p2, b, true, CHILD); // add child
      }
      getLastPointerFromSibling_page(p2, root);
      break;
  }
}
/*
 * 1 - b is before the middle
 * 2 - b is in the middle
 * 3 - b is after the middle
 */
void splitting(InternalNode *root, int b, int state, InternalNode* parent,
    InternalNode* CHILD) {
  InternalNode* p2 = new_node(root);
  int middleIndex = ORDEM - 1; //middle element when the node is full
  int tmp;
  int f;
  if (root->leaf) {
    p2->leaf = true;
  }
  switch (state) {
    case 1:
      //		printf("B está antes do elemento do meio\n");
      //		printf("Meio é o %d\n",root->index[middleIndex]);
      for (f = ORDEM; f < 2 * ORDEM; f++) {
        insert(p2, root->index[f], true, NULL); //I can delete the passPointer and substitute NULL by the pointer
        passPointer(p2, root, f);
        logicalRemove(root, f);
      }
      tmp = root->index[middleIndex];
      logicalRemove(root, middleIndex);
      if (parent) {
        p2->parent = parent;
        insert(parent, tmp, true, p2);
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert(new_root, tmp, true, NULL);
      }
      insert(root, b, true, NULL);
      getLastPointerFromSibling(p2, root);
      break;
    case 2:
      //		printf("B é o elemento do meio\n");
      middleIndex++;
      for (f = ORDEM; f < 2 * ORDEM; f++) {
        insert(p2, root->index[f], true, NULL);
        passPointer(p2, root, f);
        logicalRemove(root, f);
      }
      if (parent) {
        p2->parent = parent;
        //p2->pointer[0] = child;//rouba do parametro
        insert(parent, b, true, p2);
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert(new_root, b, true, NULL);
      }
      getLastPointerFromSibling(p2, root);
      break;
    case 3:
      //		printf("B esta depois do elemento do meio\n");
      for (f = ORDEM + 1; f < 2 * ORDEM; f++) {
        insert(p2, root->index[f], true, NULL);
        passPointer(p2, root, f);
        logicalRemove(root, f);
      }
      tmp = root->index[++middleIndex];
      logicalRemove(root, middleIndex);
      if (parent) {
        p2->parent = parent;
        insert(parent, tmp, true, p2);
        insert(p2, b, true, CHILD); // add child
      } else {
        InternalNode* new_root = new_node(NULL);
        p2->parent = new_root;
        root->parent = new_root;
        HEAD = new_root;
        insert(new_root, tmp, true, NULL);
        new_root->leaf = false;
        new_root->pointers[0] = root;
        new_root->pointers[1] = p2;
        insert(p2, b, true, CHILD); // add child
      }
      getLastPointerFromSibling(p2, root);
      break;
  }
}

//=========================================================================================
void insert_at_index_with_page(InternalNode* node, int b, bool up,
    int pageChildBySplit) {
  int i;
  for (i = 0; i < 2 * ORDEM && node->index[i] != -1 && b > node->index[i];
      i++)
    ;
  if (node->index[i] == -1) {
    insert_in_this_position_page(node, b, i, pageChildBySplit);
  } else if (b < node->index[i]) {
    if (node->stored < ORDEM * 2) {
      int j;
      for (j = i; node->index[j] != -1 && node->index[j] < b; j++)
        ;

      if (node->index[j] != -1) {
        int p;
        int next;
        int saved = -1;
        next = node->index[j];

        int nPointer = node->pages[i + 1];
        int sPointer = nPointer;

        for (p = j + 1; node->index[p] != -1 && p < ORDEM * 2; p++) {
          sPointer = node->pages[p + 1];
          node->pages[p + 1] = nPointer;
          nPointer = sPointer;

          saved = node->index[p];
          node->index[p] = next;
          next = saved;
        }
        //one more time because there are r+1 pointer for r keys
        sPointer = node->pages[p + 1];
        node->pages[p + 1] = nPointer;
        nPointer = sPointer;
        if (saved != -1) {
          node->index[p] = saved;
        } else {
          node->index[p] = next;
        }
        node->index[j] = b;
      }
      insert_in_this_position_page(node, b, j, pageChildBySplit);
    } else {
      int t;
      int middle = (2 * ORDEM + 1) / 2;
      for (t = 0; node->index[t] < b && t < middle; t++)
        ;
      if (node->index[t] > b) {
        if (t == middle) //b is the middle
          splitting2(node, b, 2, node->parent, pageChildBySplit);
        else
          //b is before the middle
          splitting2(node, b, 1, node->parent, pageChildBySplit);
      } else if (node->index[t] < b) { //b is after the middle
        splitting2(node, b, 3, node->parent, pageChildBySplit);
      }
    }
  } else if (i == 2 * ORDEM) {
    splitting2(node, b, 3, node->parent, pageChildBySplit);
  }
}

void insert(InternalNode* node, int b, bool up, InternalNode* childBySplit) {
  int i;
  for (i = 0; i < 2 * ORDEM && node->index[i] != -1 && b > node->index[i];
      i++)
    ;
  if (node->index[i] == -1) {
    if (!node->leaf && !up) {
      insert(node->pointers[i], b, false, childBySplit);
    } else
      insert_in_this_position(node, b, i, childBySplit);
  } else if (b < node->index[i]) {
    if (!node->leaf && !up) {
      insert(node->pointers[i], b, false, childBySplit);
    } else {
      if (node->stored < ORDEM * 2) {
        int j;
        for (j = i; node->index[j] != -1 && node->index[j] < b; j++)
          ;

        if (node->index[j] != -1) {
          int p;
          int next;
          int saved = -1;
          next = node->index[j];

          InternalNode* nPointer = node->pointers[i + 1];
          InternalNode* sPointer = nPointer;

          for (p = j + 1; node->index[p] != -1 && p < ORDEM * 2;
              p++) {
            sPointer = node->pointers[p + 1];
            node->pointers[p + 1] = nPointer;
            nPointer = sPointer;

            saved = node->index[p];
            node->index[p] = next;
            next = saved;
          }
          //one more time because there are r+1 pointer for r keys
          sPointer = node->pointers[p + 1];
          node->pointers[p + 1] = nPointer;
          nPointer = sPointer;
          if (saved != -1) {
            node->index[p] = saved;
          } else {
            node->index[p] = next;
          }
          node->index[j] = b;
        }
        insert_in_this_position(node, b, j, childBySplit);
      } else {
        int t;
        int middle = (2 * ORDEM + 1) / 2;
        for (t = 0; node->index[t] < b && t < middle; t++)
          ;
        if (node->index[t] > b) {
          if (t == middle) //b is the middle
            splitting(node, b, 2, node->parent, childBySplit);
          else
            //b is before the middle
            splitting(node, b, 1, node->parent, childBySplit);
        } else if (node->index[t] < b) { //b is after the middle
          splitting(node, b, 3, node->parent, childBySplit);
        }
      }
    }
  } else if (i == 2 * ORDEM) {
    if (!node->leaf && !up) {
      insert(node->pointers[i], b, false, childBySplit);
    } else {
      splitting(node, b, 3, node->parent, childBySplit);
    }
  } else if (node->index[i] == b) {
    printf("chave ja existente %d\n", b);
  }
}

void changeValue(InternalNode* p, int i, int key) {
  p->index[i] = key;
}

void logicalRemove(InternalNode* p, int i) {
  p->index[i] = -1;
  p->stored--;
  //do not remove pointers yet, they will be necessary
}

void get_total_value_file(FILE *f, int *r, int offset) {
  fseek(f, offset * sizeof((*r)), SEEK_SET);
  fread(r, sizeof((*r)), 1, f);
}

void update_write_config_file(FILE* f) {

  //writing total of pages (final nodes)
  int e = fseek(f, 0 * /* DESL. */sizeof(TOTAL_PAGES_FILE), SEEK_SET);
  if (e == 0) {
    fwrite(&TOTAL_PAGES_FILE, sizeof(TOTAL_PAGES_FILE), 1, f);
  }

  //writing total of index nodes
  e = fseek(f, 1 * sizeof(TOTAL_INDEX_FILE), SEEK_SET);
  if (e == 0) {
    fwrite(&TOTAL_INDEX_FILE, sizeof(TOTAL_INDEX_FILE), 1, f);
  }

  //writing total of records
  e = fseek(f, 2 * sizeof(TOTAL_RECORDS_FILE), SEEK_SET);
  if (e == 0) {
    fwrite(&TOTAL_RECORDS_FILE, sizeof(TOTAL_RECORDS_FILE), 1, f);
  }
}

void get_internal_node(FILE *f, InternalNode *r, int offset) {
  fseek(f, offset * sizeof(InternalNode), SEEK_SET);
  fread(r, sizeof(InternalNode), 1, f);
}

int main(void) {
  HEAD = NULL;
  TOTAL_PAGES_FILE = 0;
  TOTAL_INDEX_FILE = 0;
  TOTAL_RECORDS_FILE = 0;

  //---------------load or initialization CONFIG.txt
  FILE *fc;
  fc = fopen("config.txt", "r+");
  if (fc) {
    get_total_value_file(fc, &TOTAL_PAGES_FILE, 0);
    get_total_value_file(fc, &TOTAL_INDEX_FILE, 1);
    get_total_value_file(fc, &TOTAL_RECORDS_FILE, 2);
  } else
    TOTAL_INDEX_FILE = 0;
  //---------------load or initialization PAGES.txt
  FILE *fp;
  fp = fopen("pages.txt", "r+");
  if (!fp) {
    //=================== @TODO temporary
    remove("pages.txt"); //TEMP
    fp = fopen("pages.txt", "w+");
    //===========================
  }

  //=================== @TODO RECORDS
  FILE *fr;
  fr = fopen("records.txt", "r+");
  if (!fr) {
    remove("records.txt"); //TEMP
    fr = fopen("records.txt", "w+");
    //===========================
  }

  //---------------load or initialization INDEX.txt
  FILE *fi;
  fi = fopen("index.txt", "r");
  if (fi) {
    init_load_tree(fi);
  }

  //inicialization
  char option;
  do {
    scanf("%c%*c", &option);
    switch (option) {
      case 'i':
        btree2(fp, fr, HEAD);
        break;
      case 'c':
        search_BPlustree(fp, fr, HEAD);
        break;
      case 'r':
        remove_key(fp, fr);
        break;
      case 'f':
        final_nodes(fp, fr, HEAD);
        break;
      case 'p':
        init_print_tree(fp, fr, HEAD);
        break;
    }
  } while (option != 'e');

  //=================== @TODO temporary
  if (fi) {
    fclose(fi);
    remove("index.txt");
  }
  fi = fopen("index.txt", "w+");
  if (fi) {
    TOTAL_INDEX_FILE = 0; //important because we will rewrite all the file
    //and start at 0
    store_index(fi, HEAD);
  }
  fclose(fi);
  //===========================
  if (fc) {
    fclose(fc);
    remove("config.txt");

  }
  fc = fopen("config.txt", "w+");
  update_write_config_file(fc);
  fclose(fc);

  //never contract, just expand
  fclose(fp);
  fclose(fr);
  return 0;
}

