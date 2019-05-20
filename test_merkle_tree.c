#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
unsigned int FILESIZE_MAX = 500;

static int filesys_inited = 0;
struct merkle_node{
    struct merkle_node* left;
    struct merkle_node* right;
    char hash[20];
};

/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
    SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

void combine(struct merkle_node* sha1, struct merkle_node* sha2, struct merkle_node* sha3){

    char* temp = malloc(40);
    memcpy(temp,&(sha1->hash),20);
    memcpy(temp+20,&(sha2->hash),20);
    get_sha1_hash(temp, 40, sha3->hash);
    sha3->left = sha1;
    sha3->right = sha2;

//    printf(sha3->left->hash);
}

// len is number of leaves.
void get_merkle_root (const char *hashes, int len, void *root)
{
    struct merkle_node *nodes = malloc(len * sizeof(struct merkle_node));
    for(int i = 0; i < len;i++){
        //would an arrow work?
        memcpy( &( ( *(nodes + i) ).hash), (hashes + 20*i), 20);
    }

    while(1) {
        int len_above = 0;
        struct merkle_node *nodes_above = malloc(len * sizeof(struct merkle_node));
        for (int i = 0; i < len - 1; i += 2) {
            combine(nodes + i, nodes + i + 1, nodes_above + (i/2));
//            printf("\nhashes are \n");
//            printf((nodes_above + (i/2))->hash);
//            printf((nodes_above + (i/2))->left->hash);
            len_above++;
        }
        if (len % 2) {
            len_above++;

            struct merkle_node *parent = nodes_above + len_above -1;
            struct merkle_node *child = nodes + (len - 1);
            memcpy( &( ( *(nodes_above + len_above - 1) ).hash),  &( ( *(nodes + len - 1) ).hash), 20);
            (nodes_above + len_above -1)->left = (nodes + (len - 1));
//            printf("Hashes of memcopy\n");
//            printf(((nodes_above + len_above - 1) )->hash);
//            printf("\n");
//            printf(((nodes + len - 1) )->hash);
//            printf("\n");

        }

        if (len_above == 1) {
            memcpy(root, nodes_above, sizeof(struct merkle_node));
            printf("length is: %d\n",len_above);
            return;
        }
        len = len_above;
        //free(hashes);
        printf("length is: %d\n",len_above);
        nodes = nodes_above;
    }

}

int get_merkle_height(struct merkle_node *root){
    int height = 0;
//    struct merkle_node * ptr = root;
    while (1){
        if (root == NULL){
            return  height;
        }
        height++;
        root = root->left;
    }
}

struct merkle_node *get_leaf(struct merkle_node *root,int pos){
    int height = get_merkle_height(root);

    int approx_tot = 1LL<<(height - 1) ;
    int start =1,end = approx_tot;

    while (1){
        int mid = (start+end) / 2;
        if (start == end){
            return root;
        }
        if (pos <= mid){
            end = mid;
            root = root->left;
        }
        else{
            start = mid+1;
            root = root->right;
        }
    }
}

struct merkle_node* merkle_insert(struct merkle_node *root, struct merkle_node *new_node){
    struct merkle_node *right_available = NULL;

    while(1){
        if(root->right == NULL && root->left == NULL){
            break;
        } else if (root->right == NULL){
            right_available = root;
            root = root->left;
        } else{
            root = root->right;
        }
    }


    if(right_available == NULL){
        right_available = malloc(sizeof(struct merkle_node));
        memcpy(&(right_available->hash), &(root->hash), 20);
        right_available->left = root;
        root = right_available;
    }

    int height = get_merkle_height(right_available);

    combine(right_available->left, new_node, right_available);
    while(height - 2){ //recheck
        struct merkle_node *cur = malloc(sizeof(struct merkle_node));
        new_node->left = cur;
        height--;
        memcpy(&(cur->hash), &(new_node->hash), 20);
        new_node = cur;
    }


    return root;
}
int compare_hash(const char* hash1, const char* hash2){
    for (int i = 0; i < 20; ++i) {
        if (*(hash1 + i) != *(hash2+i)){
            return 1;
        }
    }

    return 0;
}

int main(){
//    int fdtest = open("savit.txt",O_APPEND|O_CREAT);
//    char* pathtest = malloc(30);

//    readlink()

    char* hashes_local = malloc(105);
    char arr[5] = { 1, 2, 3, 4, 5 };

    for (int i = 0; i < 5; ++i) {
        char * buf = malloc(2);
        *buf = arr[i];
        get_sha1_hash(buf,1,hashes_local+i*20);
    }
    struct merkle_node* root = malloc(25);
    get_merkle_root(hashes_local,5,root);
//    char* savit = "savitgupta";
//    printf(savit);
//    printf(root->hash);
//    printf(root->left->hash);
//    printf(root->left->left->hash);

    char* hashes1 = malloc(20);
    char* hashes2 = malloc(20);
    char hashes3[20];
    char  hashes4[20];

    for (int j = 0; j < 20; ++j) {
        *(hashes1 + j) = (char) 66;
        *(hashes2 + j) = (char) 66;
        hashes3[j] = (char) 56;
        hashes4[j] = (char) 66;
    }
    printf("compare hash result %d \n",compare_hash(hashes4,hashes3));
    printf("compare hash result %d \n",compare_hash(hashes1,hashes3));

    printf("\nFinished\n");

    struct merkle_node *leafnode = get_leaf(root,2);

    printf("Leaf node \n");

    struct merkle_node *add_node = malloc(25);
    char * buf = malloc(2);
    *buf = 6;
    get_sha1_hash(buf,1,add_node->hash);

    merkle_insert(root,add_node);

    *buf = 7;
    get_sha1_hash(buf,1,add_node->hash);

    merkle_insert(root,add_node);
    printf("Check merkle finally\n");
    //    for (int j = 0; j < 20; ++j) {
//        printf(*(root + j));
//    }

}