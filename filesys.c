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
#include "errno.h"

unsigned int FILESIZE_MAX = 5000;
unsigned int FNSIZE = 10;


static int filesys_inited = 0;
struct merkle_node{
    struct merkle_node* left;
    struct merkle_node* right;
    char hash[20];

};

struct merkle_node *fd_to_rootarr[256];
char *fd_to_filename[256];

int min(int a, int b){
    if(a > b) return b;
    return a;
}
int max(int a, int b)
{
    return a + b - min(a, b);
}

/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
    SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

int hash_is_zero(char *hash) {
    for (int i = 0; i < 20; ++i) {
        if (hash[i] != 0 ){
            return 0;
        }
    }
    return 1;
}

int hash_difference(const char* hash1, const char* hash2){
    for (int i = 0; i < 20; ++i) {
        if (*(hash1 + i) != *(hash2+i)){
            return 1;
        }
    }

    return 0;
}
void print_hash(struct merkle_node *root, char *message){
    printf("%s \n",message);
    for (int i = 0; i < 20; i++) {
        printf("%d ", (int)*(root->hash+i));
    }
    printf("\n");
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
void get_merkle_root (const char *hashes, int len, struct merkle_node *root)
{
    struct merkle_node *nodes = malloc(len * sizeof(struct merkle_node));
    for(int i = 0; i < len;i++){
        //would an arrow work?
//        memcpy( &( ( *(nodes + i) ).hash), (hashes + 20*i), 20);
        memcpy( &( ( *(nodes + i) ).hash), (hashes + 20*i), 20);
    }
    if (len == 0){
//        printf("before: assigned null to hash\n");
        memset(&(root->hash), 0, 20);
//        printf("assigned null to hash\n");
        return;
    }
    while(1) {
        int len_above = 0;
        struct merkle_node *nodes_above = malloc(len * sizeof(struct merkle_node));
        for (int i = 0; i < len - 1; i += 2) {
            combine(nodes + i, nodes + i + 1, nodes_above + (i/2));
            len_above++;
        }
        if (len % 2) {
            len_above++;
            memcpy( &( ( (nodes_above + len_above - 1) )->hash),  &( ( (nodes + len - 1) )->hash), 20);
            (nodes_above + len_above -1)->left = (nodes + (len - 1));


        }

        if (len_above == 1) {
            memcpy(root, nodes_above, sizeof(struct merkle_node));
//            printf("length is: %d\n",len_above);
            return;
        }
        len = len_above;
//        printf("length is: %d\n",len_above);
        nodes = nodes_above;
    }

}

int get_merkle_height(struct merkle_node *root);

void write_modify(__off_t position, struct merkle_node *root, const char *hash) ;

void destroy_tree(struct merkle_node *pNode);


int hash_is_zero(char *hash);

//returns the new root of the merkle_tree
struct merkle_node* merkle_insert(struct merkle_node *root, struct merkle_node *new_node){
    struct merkle_node *right_available = NULL;
    if(hash_is_zero(root->hash)){
        return new_node;
    }
    struct merkle_node *node_stack[100];
    int i = 0;
    struct merkle_node* root_copy = root;
    while(1){
        node_stack[i] = root_copy;
        i++;
        if(root_copy->right == NULL && root_copy->left == NULL){
            break;
        } else if (root_copy->right == NULL){
            right_available = root_copy;
            root_copy = root_copy->left;
        } else{
            root_copy = root_copy->right;
        }
    }

    int right_isnull = 0;
    if(right_available == NULL){
        right_available = malloc(sizeof(struct merkle_node));
        memcpy(&(right_available->hash), &(root->hash), 20);
        right_available->left = root;
        root = right_available;
        node_stack[i] = right_available;
        i++;
        right_isnull = 1;
    }





    int height = get_merkle_height(right_available);

    combine(right_available->left, new_node, right_available);
    if(!right_isnull){
        i--;//popping those that came after right_available
        while(node_stack[i] != right_available){
            i--;
        }

        while(i > 0){
            i--;
            if(node_stack[i]->right == NULL){
                memcpy(&(node_stack[i]->hash), &(node_stack[i]->left->hash), 20);
                continue;
            }
            combine(node_stack[i]->left, node_stack[i]->right, node_stack[i]);
        }
    }


    while(height - 2){
        //rehash
        struct merkle_node *cur = malloc(sizeof(struct merkle_node));
        new_node->left = cur;
        height--;
        memcpy(&(cur->hash), &(new_node->hash), 20);
        new_node = cur;
    }


    return root;
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
        int mid = (start+end) >> 1;
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

struct merkle_node *get_root_from_fd_securetxt(int secfd, const char *pathname) ;

struct merkle_node *get_root_from_securetxt(const char *pathname) {

    int fd = open("secure.txt", O_RDWR);
    if (fd == -1){
        return NULL;
    }
    struct merkle_node *ret = get_root_from_fd_securetxt(fd,pathname);
    close(fd);
    return ret;

}
struct merkle_node *get_root_from_fd_securetxt(int secfd, const char *pathname) {

    __off_t initial_offset = lseek(secfd,0,SEEK_CUR);

    int rootnode_size = sizeof(struct merkle_node);
    int count =0,size_of_entry = FNSIZE + rootnode_size;

    // end of the file
    __off_t end_num = lseek(secfd,0,SEEK_END);
    lseek(secfd,0,SEEK_SET);

    char* filepath = malloc(FNSIZE);
    struct merkle_node *rootnode = malloc(rootnode_size);
//    printf("get root from fd secure txt : %d %d  error no: %d\n", (int)end_num, size_of_entry,errno);

    while (count + size_of_entry <= end_num){
        count += size_of_entry;

        read(secfd,filepath,FNSIZE);
        read(secfd,rootnode,rootnode_size);

        if (strcmp(pathname,filepath)==0){
            lseek(secfd,initial_offset,SEEK_SET);
            return rootnode;
        }
    }
    lseek(secfd,initial_offset,SEEK_SET);
    return NULL;
}
struct merkle_node* create_merkle_tree(int fd){
    struct merkle_node *root1 = malloc(sizeof(struct merkle_node));
    char* temp_buf = malloc(64);
    __off_t initial_count = lseek(fd,0,SEEK_CUR);
    __off_t end_count = lseek(fd,0,SEEK_END);
    char* leaves = malloc((size_t) ((end_count / 64) * 20 + 500));

    int curr_count = 0;


    lseek(fd,0,SEEK_SET);

//    printf("create_merkle_tree: leaves is \n");

    while(curr_count < end_count){
        ssize_t bytes_read = 0;

        if((bytes_read = read (fd, temp_buf, 64)) == 0){
            break;
        }

        get_sha1_hash(temp_buf,bytes_read, leaves + 20*(curr_count/64));
        curr_count += 64;
    }
    get_merkle_root(leaves, curr_count/64, root1);

    lseek(fd,initial_count,SEEK_SET);
//    printf("returning generated root of filename: %s \n",fd_to_filename[fd]);
    return root1;
}
/* Build an in-memory Merkle tree for the file.
 * Compare the integrity of file with respect to
 * root hash stored in secure.txt. If the file
 * doesn't exist, create an entry in secure.txt.
 * If an existing file is going to be truncated
 * update the hash in secure.txt.
 * returns -1 on failing the integrity check.
 */
int s_open (const char *pathname, int flags, mode_t mode)
{
    assert (filesys_inited);
    //TODO: account for truncation
    flags = ( flags & ~O_WRONLY) | O_RDWR;
    int fd = open(pathname,flags,mode);
    fd_to_filename[fd] = malloc(FNSIZE);
    strcpy(fd_to_filename[fd], pathname);

    struct merkle_node *root1 = create_merkle_tree(fd);

    struct merkle_node *root = get_root_from_securetxt(pathname);

    if(root == NULL){
        int secfd = open("secure.txt",O_RDWR);
        lseek(secfd,0,SEEK_END);
        if(secfd == -1 ) {
            printf("ERROR errno is %d \n", errno);
        }
        write(secfd,pathname,FNSIZE);
        write(secfd,root1, sizeof(struct merkle_node));
        root = root1;

        close(secfd);
    }


    if(hash_difference(root1->hash, root->hash) != 0){
        return -1;
    }
    root = root1;
    fd_to_rootarr[fd] = root;
    lseek(fd,0,SEEK_SET);
    if(flags & O_TRUNC)
    {
        memset(&(root->hash), 0, 20);
        root->right = NULL;
        root->left = NULL;
        fd_to_rootarr[fd] = root;
    }
    return fd;
}


int get_merkle_byte_count(struct merkle_node *root) {
    if(root->left == NULL && root->right == NULL) return 1LL << 6;
    int lcount = 0, rcount = 0;
    if(root->left != NULL)
    {
        lcount = get_merkle_byte_count(root->left);
    }
    if(root->right != NULL)
    {
        rcount = get_merkle_byte_count(root->right);
    }
    return lcount + rcount;
}

/* SEEK_END should always return the file size
 * updated through the secure file system APIs.
 */
int s_lseek (int fd, long offset, int whence)
{
    assert (filesys_inited);
//    return lseek (fd, offset, SEEK_SET);
    if(whence == SEEK_END) {
        struct merkle_node *root = fd_to_rootarr[fd];
        int byte_count = get_merkle_byte_count(root);
        return lseek(fd, byte_count, SEEK_SET);
    }
    else
    {
        return lseek(fd, offset, whence);
    }
}

void write_to_secure(int fd){

    int secfd = open("secure.txt", O_RDWR);
    if(secfd == -1){
//        printf("write_to_secure: unable to open secure.txt\n");
        assert(0);
    }
    int rootnode_size = sizeof(struct merkle_node);
    int count =0,size_of_entry = FNSIZE + rootnode_size;
    //printf("write to secure: secfd opened %d\n", secfd);
    // end of the file
    __off_t end_num = lseek(secfd,0,SEEK_END);
    lseek(secfd,0,SEEK_SET);

    char* filepath = malloc(FNSIZE);
    struct merkle_node *rootnode = malloc(rootnode_size);
    while (count + size_of_entry <= end_num){
        count += size_of_entry;

        read(secfd,filepath,FNSIZE);
        if (strcmp(filepath,fd_to_filename[fd]) == 0){
            write(secfd,fd_to_rootarr[fd],rootnode_size);
            break;
        }

        read(secfd,rootnode,rootnode_size);

    }
//    printf("write to secure: almost ended\n");
    lseek(secfd,0,SEEK_SET);
    close(secfd);
}


/* read the blocks that needs to be updated
 * check the integrity of the blocks
 * modify the blocks
 * update the in-memory Merkle tree and root in secure.txt
 * returns -1 on failing the integrity check.
 */
ssize_t s_write (int fd, const void *buf, size_t count)
{
    //TODO: Check when count % 64 != 0

    assert (filesys_inited);
    __off_t ini_pos = lseek(fd,0,SEEK_CUR);
    __off_t endpos = lseek(fd,0,SEEK_END);
//    char* buf1 = (char*)buf;
    lseek(fd, ini_pos - ini_pos%64, SEEK_SET); //ini_pos%64 are initial bytes we need to add
    char* new_buf = malloc(count + 128);
    int tempx = read(fd, new_buf, (size_t) (ini_pos % 64));//reading the appropriate amount of first block
    if(tempx == -1){
//        printf("%d\n",errno);
    }
    memcpy(new_buf + ini_pos%64, buf, count); //copying the buf to the new_buf

    int read_from_eof = max(min((64 - (ini_pos + count)%64), endpos - ini_pos - count), 0);
    lseek(fd, ini_pos + count, SEEK_SET); // sets offset to what it would be after writing count bytes.
    read(fd, new_buf + count + ini_pos % 64, read_from_eof);
//    memset(new_buf + count + ini_pos % 64 + read_from_eof, 0 ,  (64 - (ini_pos + count)% 64)%64 - read_from_eof);


    lseek(fd, ini_pos - ini_pos%64, SEEK_SET);

    size_t new_cnt = count + ini_pos % 64 + read_from_eof;

    size_t modify_end = ((new_cnt > endpos - (ini_pos - ini_pos%64)) ? endpos - (ini_pos - ini_pos%64) : new_cnt);

    char* temp_buf = malloc(64);
    char * hash = malloc(20);

    int write_cnt=0;

    while(write_cnt < modify_end){ // TODO recheck equality
//        printf("s_write: Modify While loop entered\n");
        ssize_t bytes_read = 0;

        //if the file has ended
        if((bytes_read = read (fd, temp_buf, 64)) == 0){
//            printf("s_write: should not reach, no bytes left\n");
            assert(0);
//            break;
        }
        get_sha1_hash(temp_buf,bytes_read, hash);

        //-1 because we've already read ahead
        __off_t position = ( ( lseek(fd,0,SEEK_CUR) + 63 ) / 64 );

        struct merkle_node *root = fd_to_rootarr[fd];

        struct merkle_node *current_node =  get_leaf(root,position);
        if(hash_difference(hash,current_node->hash)!=0){
            lseek(fd,ini_pos,SEEK_SET);
            return -1;
        }
        char *new_hash = malloc(20);
        get_sha1_hash(new_buf+write_cnt, min(64, (int) (new_cnt - write_cnt)), new_hash);
        write_modify(position,root,new_hash);

        write_cnt += 64;
    }
    //append

    while(write_cnt < new_cnt){
        //write to buffer
        struct merkle_node *add_node = malloc(sizeof(struct merkle_node));


        get_sha1_hash(new_buf + write_cnt, min(64, (int) (new_cnt - write_cnt)), add_node->hash);
        struct merkle_node *root = fd_to_rootarr[fd];
//        print_hash(add_node,"s_write: new node hash is");
        fd_to_rootarr[fd] = merkle_insert(root,add_node);
//        printf("Inserted correctly\n");
//        root = fd_to_rootarr[fd];
//        print_hash(root,"s_write: now root hash is");
        write_cnt += 64;
    }
//    printf("s_write before writing to secure\n");

    write_to_secure(fd);
    lseek(fd, ini_pos, SEEK_SET);
    return write (fd, buf, count);
}

void write_modify_recursion(int pos, int start, int end, struct merkle_node *root, const char *hash){
    if (start == end){
        memcpy(&(root->hash),hash, sizeof(root->hash));
        return;
    }

    int mid = (start + end) >> 1;

    if (pos <= mid){
        write_modify_recursion(pos,start,mid,root->left,hash);
    } else{
        write_modify_recursion(pos,mid+1,end,root->right,hash);
    }

    if(root->right == NULL){
        memcpy(&(root->hash), &(root->left->hash), 20);
    }
    else{
        combine(root->left,root->right,root);
    }
}


void write_modify(__off_t position, struct merkle_node *root, const char *hash) {


    int height = get_merkle_height(root);
    int approx_tot = 1LL<<(height - 1) ;
    int start =1,end = approx_tot;

    write_modify_recursion(position, start, end, root, hash);


}




/* check the integrity of blocks containing the
 * requested data.
 * returns -1 on failing the integrity check.
 */
ssize_t s_read (int fd, void *buf, size_t count)
{
    assert (filesys_inited);
//    printf("Count: ")
    char * hash = malloc(20);

    int i = 0;
    char* temp_buf = malloc(64);

    int end_cnt = 0;

    __off_t curpos = lseek(fd,0,SEEK_CUR);
//    printf("Curpos is: %d\n", (int) lseek(fd,0,SEEK_CUR));
    if ((curpos % 64 )!= 0){
        lseek(fd,-(curpos%64),SEEK_CUR);
    }

    while(end_cnt<count){
        ssize_t bytes_read = 0;

        //if the file has ended
        if((bytes_read = read (fd, temp_buf, 64)) == 0){
            break;
        }
        get_sha1_hash(temp_buf,bytes_read, hash);

        __off_t position =( lseek(fd,0,SEEK_CUR) / 64 );


        struct merkle_node *root = fd_to_rootarr[fd];

        struct merkle_node *current_node =  get_leaf(root,position);
//        printf("s_read: starting compare\n");
        if(hash_difference(hash,current_node->hash)!=0){
            lseek(fd,curpos,SEEK_SET);
            return -1;
        }
//        memcpy(leaves + i, hash, 20);

        i += 20;
        end_cnt += 64;
    }

    lseek(fd,curpos,SEEK_SET);
//    printf("gets here\n");
//    printf("Count is: %d\n", (int) lseek(fd,0,SEEK_CUR));
    return read (fd, buf, count);
}

/* destroy the in-memory Merkle tree */
int s_close (int fd)
{
    assert (filesys_inited);
//    printf("Sclose started\n");
//    struct merkle_node *tempelem = malloc(sizeof(struct merkle_node));
//    memcpy(tempelem,fd_to_rootarr[fd])
//    struct merkle_node* temp = fd_to_rootarr[fd];
//    print_hash(temp, "s_close: to be destoryed");
    destroy_tree(fd_to_rootarr[fd]);
//    printf("Sclose completed\n");

    fd_to_rootarr[fd] = NULL;
    return close (fd);
}

void destroy_tree(struct merkle_node *root) {
//    printf("Destroying\n");
    if (root->left == NULL && root->right == NULL){
//        printf("Freeing leaf 1\n");
//        print_hash(root,"Leaf node to be freed");
//        free(root);
//        printf("Freed leaf 2\n");
        return;
    }

    destroy_tree(root->left);
    if(root->right != NULL){
//        printf("Going to right\n");
        destroy_tree(root->right);
    }
    root->left = NULL;
    root->right = NULL;

//    free(root);
}


/* Check the integrity of all files in secure.txt
 * remove the non-existent files from secure.txt
 * returns 1, if an existing file is tampered
 * return 0 on successful initialization
 */
int filesys_init (void)
{
    filesys_inited = 1;

    int rootnode_size = sizeof(struct merkle_node);

    int fd = open("secure.txt",O_CREAT | O_RDWR,0777);

    int count =0,size_of_entry = FNSIZE + rootnode_size;

    // end of the file
    __off_t end_num = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);

    char* filepath = malloc(FNSIZE);
    struct merkle_node *rootnode = malloc(rootnode_size);

    char* filestoput = malloc(end_num);
    int newsize =0;
    while (count + size_of_entry <= end_num){
//        printf("Goes here\n");
        count += size_of_entry;

        read(fd,filepath,FNSIZE);

        read(fd,rootnode,rootnode_size);
        int temp_fd = open(filepath,O_RDONLY);
        if (temp_fd == -1){
            continue;
        }
        if(temp_fd != -1){
            memcpy(filestoput+newsize*size_of_entry , filepath,FNSIZE);
            memcpy(filestoput+newsize*size_of_entry + FNSIZE, rootnode,rootnode_size);
            newsize++;
        }

        struct merkle_node* root1 = create_merkle_tree(temp_fd);
        struct merkle_node* root = get_root_from_fd_securetxt(fd,filepath);
        if (root == NULL){
//            printf("filesys_init: null root fd is:%d\n",fd);
            assert(0);
        }
        if(hash_difference(root1->hash, root->hash) != 0){
            return -1;
        }

        close(temp_fd);
    }
    if(fd == -1){
//        printf("filesys_init:Error Number is %d \n", errno);
    }
    lseek(fd,0,SEEK_SET);
    write(fd,filestoput,newsize*size_of_entry);

    ftruncate(fd,newsize*size_of_entry);
    return 0;
}