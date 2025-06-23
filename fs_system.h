
#include "fs_server.h"
#include <boost/thread.hpp>
#include "fs_client.h"
#include "fs_param.h"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unordered_map>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <set>



/*export CPLUS_INCLUDE_PATH=/usr/local/boost/include:
export LIBRARY_PATH=/usr/local/boost/lib:
export LD_LIBRARY_PATH=/usr/local/boost/lib:*/

/*struct direntries_wrapper{
    
    uint32_t file_block_num;
    std::vector<fs_direntry> direntries;
    uint32_t size;
};*/

//Mutexes for data structures
//boost::mutex all_inodes_mutex;
/*boost::mutex all_direntries_mutex;
boost::mutex available_disk_blocks_mutex;
boost::mutex tree_mutex;
boost::mutex all_names_mutex;
boost::mutex mutex_map_mutex;*/

//MUTEX FOR IN-MEMORY DATA STRUCTURES
boost::mutex ds_mutex;


//TODO:
//1. Create a structure to store fs_inodes

//std::vector<fs_inode> all_inodes;
//std::vector<direntries_wrapper> all_direntries;

//2. A way to link an direntry back to its corresponding fs_inode

//Index is the block # out of 4096 available, and the value is the index of the inode in the all_inodes vector
//std::vector<uint32_t> block_to_inode;
//std::vector<uint32_t> block_to_direntries;

std::vector<uint32_t> available_disk_blocks;
std::unordered_map<uint32_t, std::shared_ptr<boost::shared_mutex>> locks;


/*struct TreeNode{
    
    std::shared_ptr<TreeNode> parent; //parent node
    std::string name;
    char type; //file or directory
    uint32_t block; //block number of the node
    uint32_t direntry_block;
    std::shared_ptr<boost::shared_mutex> mutex; //mutex for this node

    //give it children: dictionary of nodes. key is f/d name and TreeNode is node it pts to
    std::unordered_map<std::string, std::shared_ptr<TreeNode>> tree_children;


};*/

//fs_inode root_node;

//std::set<std::string> all_names;


uint16_t server_port;
const char* server_hostname;
sockaddr_in addr{};

std::unordered_map<std::string, std::shared_ptr<boost::shared_mutex>> mutex_map;

uint16_t parse_line(int argc, char *argv[]);

int init_server(uint16_t port);

void set_used_blocks(uint32_t block_num, std::set<uint32_t>& used_blocks);
int find_duplicate(fs_inode main, std::string fname);

std::shared_ptr<char[]> handle_readblock(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], uint32_t block, int &status);
int handle_writeblock(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], uint32_t block, void* data, size_t data_len);
int handle_create(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], char type);
int handle_delete(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1]);
void handle_request(int client_socket);
std::vector<std::string> char_array_to_string_vector(char char_array[FS_MAXFILENAME + 1]);
int traverse_tree(std::vector<std::string> path_vector, bool write_child, uint32_t& child_block, uint32_t& parent_block, char username_char[FS_MAXUSERNAME + 1]);
int traverse_tree_create(std::vector<std::string> path_vector, uint32_t& parent_block, char username_char[FS_MAXUSERNAME + 1]);
int traverse_tree_delete(std::vector<std::string> path_vector, uint32_t& child_block, uint32_t& parent_block,  char username_char[FS_MAXUSERNAME + 1]);
