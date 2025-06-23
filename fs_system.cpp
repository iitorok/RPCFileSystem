#include "fs_system.h"
#include <unistd.h>

/*SET_USED_BLOCKS
--------------------------------------------------------------------
->A helper function we use in our init_server function to collect all of the
->blocks currently in use by an existing filesystem image that MAY EXIST
->Traverses the existing filesystem and puts the blocks in use into used_blocks. If
the block is in this set, we don't add it to available_disk_blocks in init_server.
--------------------------------------------------------------------*/

void set_used_blocks(uint32_t block_num, std::set<uint32_t>& used_blocks) {

    std::vector<uint32_t> blocks_left;

    if (used_blocks.count(block_num) == 0) {
        blocks_left.push_back(block_num);
    }

    while (!blocks_left.empty()) {
        uint32_t current_block_num = blocks_left.back();
        blocks_left.pop_back();

        if (used_blocks.count(current_block_num) > 0) {
            continue;
        }

        locks.emplace(current_block_num, std::make_shared<boost::shared_mutex>());

        used_blocks.insert(current_block_num);

        fs_inode node;
        
        char inode_buf[FS_BLOCKSIZE];
        memset(inode_buf, 0, FS_BLOCKSIZE);
        disk_readblock(current_block_num, inode_buf);
        
        memcpy(&node, inode_buf, sizeof(fs_inode));

        for (uint32_t i = 0; i < node.size; ++i) {
            uint32_t data_block_num = node.blocks[i];

            if (data_block_num != 0) {
                used_blocks.insert(data_block_num);

                if (node.type == 'd') {

                    char dir_block_buf[FS_BLOCKSIZE];
                    memset(dir_block_buf, 0, FS_BLOCKSIZE);
                    disk_readblock(data_block_num, dir_block_buf);
                    fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

                    for (int j = 0; j < 8; ++j) {
                        uint32_t next_inode_block = direntries[j].inode_block;

                        if (next_inode_block != 0 && used_blocks.count(next_inode_block) == 0) {

                           blocks_left.push_back(next_inode_block);
                        }
                    }
                }
            } 

        }

    }
}


/*FIND_DUPLICATE
--------------------------------------------------------------------
->A helper function we use in handle_create to determine if a created file/directory
already exists in a given path and if it does, the function returns -1.
--------------------------------------------------------------------*/

int find_duplicate(fs_inode main, std::string fname){
    //Traverse all the direntries
    
    for(uint32_t i = 0; i < main.size; ++i){

        char dir_block_buf[FS_BLOCKSIZE];
        memset(dir_block_buf, 0, FS_BLOCKSIZE);
        disk_readblock(main.blocks[i], dir_block_buf);
        fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

        for(int i = 0; i < 8; ++i){
            
            if(strcmp(direntries[i].name, fname.c_str()) == 0){
                //FOUND A DUPLICATE!
                return -1;
            }
        }
    }
    return 0;
}

/*HANDLE_REQUEST
-----------------------------------------------------------
->We use this function to handle each of the four requests made by the client.
->First, we receive the client's message and read in the 'type',
handling each of the four types of requests accordingly.
->If any of the helper-handler functions fail, handle_request
checks and the socket is closed without sending a message.
-----------------------------------------------------------*/

void handle_request(int client_socket){
    
    char msg[64] = {}; 

    std::string message;
    int return_val = 0;
    uint32_t total_bytes = 0;
    int max_message_len = FS_BLOCKSIZE + 3 + FS_MAXFILENAME + FS_MAXPATHNAME + FS_MAXUSERNAME + 13 + 3;
   
    do {     
        return_val = recv(client_socket, msg, 64, 0);
        total_bytes += return_val;
       
        if(return_val == 0){
            break;
        }else if(return_val < 0){
        
            break;
        }
            
        message += std::string(msg, return_val);

    } while ((return_val == 64) && (message[message.length() - 1] != '\0') && (message.length() < max_message_len));

    if(message.length() > max_message_len){
        close(client_socket);
        return;
    }

    std::istringstream istr(message);
    std::istringstream testist(message);

    std::string token;

    std::vector<std::string> command;

   while(std::getline(testist, token, ' ')) {
        command.push_back(token);

        if(token == "FS_WRITEBLOCK") {
            break;
        }

        if(command.size() > 4) {
            close(client_socket);
            return;
        }
    }

    if(command[0] == "FS_READBLOCK") {
        if(command.size() != 4) {
            close(client_socket);
            return;
        }
    }
    
    if(command[0] == "FS_CREATE") {
        if(command.size() != 4) {
            close(client_socket);
            return;
        }
    }

    if(command[0] == "FS_DELETE") {
        if(command.size() != 3) {
            close(client_socket);
            return;
        }
    }

    if(command[0] != "FS_READBLOCK" && command[0] != "FS_WRITEBLOCK" && command[0] != "FS_CREATE" && command[0] != "FS_DELETE") {
        close(client_socket);
        return;
    }


    
    std::string type;
    std::string usernm;
    std::string pathnm;
        
    if (!(istr >> type)) {
        close(client_socket);
        return;
    }
    char typeArray[type.length() + 1];
    std::strcpy(typeArray, type.c_str());

    if (!(istr >> usernm)) {
        close(client_socket);
        return;
    }

    if (usernm.length() > FS_MAXUSERNAME) {
        close(client_socket);
        return;
    }
    char usernmArray[usernm.length() + 1];
    std::strcpy(usernmArray, usernm.c_str());

    if (!(istr >> pathnm)) {
        close(client_socket);
        return;
    }
    if (pathnm.length() > FS_MAXPATHNAME) {
         close(client_socket);
         return;
    }
    char pathnmArray[pathnm.length() + 1];
    std::strcpy(pathnmArray, pathnm.c_str());

        
    if(type == "FS_READBLOCK") {

        uint32_t block_int;
        std::string block_string;

        std::string temp;
        std::string temp_message;
        std::vector<std::string> temp_vector;

        std::istringstream istr2(message);

        std::getline(istr2, temp_message, '\0');
        
        std::istringstream istr3(temp_message);
        
        while(std::getline(istr3, temp, ' ')) {
            temp_vector.push_back(temp);
        }

        block_string = temp_vector.back();

        if(block_string.empty()) {
            close(client_socket);
            return;
        }

        if(block_string[0] == '0' && block_string.length() > 1) {
            close(client_socket);
            return;
        }

        block_int = std::stoul(block_string);

        if(block_int >= FS_MAXFILEBLOCKS) {
            close(client_socket);
            return;
        }

        int status = 0;
        
        
        std::shared_ptr<char[]> data = handle_readblock(usernmArray, pathnmArray, block_int, status);
     
        if(status != -1){
            
            message.append(data.get(), FS_BLOCKSIZE);
      
            size_t bytes_sent = 0;
           
            do {
                bytes_sent += send(client_socket, message.c_str() + bytes_sent, 
                message.length() - bytes_sent, 0);
            
            } while ((bytes_sent < message.length()));
        }
            
    }else if(type == "FS_WRITEBLOCK") {

        uint32_t block_int;
        std::string block_string;

        std::string temp;
        std::string temp_message;
        std::vector<std::string> temp_vector;

        std::istringstream istr2(message);
     
        std::getline(istr2, temp_message, '\0');
        
        std::istringstream istr3(temp_message);
      
        while(std::getline(istr3, temp, ' ')) {
          
            temp_vector.push_back(temp);
            if(temp_vector.size() > 4) {
               
                close(client_socket);
                return;
            }
        }

        if(temp_vector.size() != 4) {
            close(client_socket);
            return;
        }

        block_string = temp_vector.back();

        if(block_string.empty()) {
            close(client_socket);
            return;
        }
      
        if(block_string[0] == '0' && block_string.length() > 1) {
            close(client_socket);
            return;
        }

        block_int = std::stoul(block_string);


        if(block_int >= FS_MAXFILEBLOCKS) {
            close(client_socket);
            return;
        }

        if(total_bytes < FS_BLOCKSIZE) {
            close(client_socket);
            return;
        }

        char* null_pos = (char*)memchr(message.c_str(), '\0', total_bytes);

        std::string command = message.substr(0, null_pos - message.c_str());

        char* data = null_pos + 1;
        

        size_t command_len = null_pos - message.c_str();
        command_len++;
        
        size_t data_len = message.length() - command_len;
        
        if(handle_writeblock(usernmArray, pathnmArray, block_int, data, data_len) == 0) {
            
            size_t bytes_sent = 0;
            do {
                bytes_sent += send(client_socket, message.c_str() + bytes_sent, 
                command_len - bytes_sent, 0);
            } while (bytes_sent < command_len); 

        }

    }else if(type == "FS_CREATE") {
     
        std::string file_type;
        istr >> file_type;
        char file_typeArray[file_type.length() + 1];
        std::strcpy(file_typeArray, file_type.c_str());
        char file_type_char = file_typeArray[0];

        if(file_type_char != 'f' && file_type_char != 'd') {
            close(client_socket);
            return;
        }

      
        if(handle_create(usernmArray, pathnmArray, file_type_char) == 0) {
            size_t bytes_sent = 0;
            do {
                bytes_sent += send(client_socket, message.c_str() + bytes_sent, 
                message.length(), 0);
            } while (bytes_sent < message.length());
        }


    } else if(type == "FS_DELETE") {
    
        //The response message for a successful FS_DELETE is the same as the request message.

        if(handle_delete(usernmArray, pathnmArray) == 0) {
            size_t bytes_sent = 0;
            do {
                bytes_sent += send(client_socket, message.c_str() + bytes_sent, 
                message.length(), 0);
            } while (bytes_sent < message.length());
        }

    }else{
        
        //Close the client socket
    }  

    close(client_socket); 
}


int main(int argc, char *argv[]) {

    //Get the port number
    uint16_t port = parse_line(argc, argv);
 
    init_server(port);

}

/*INIT_SERVER
-------------------------------------------------
-> Function we use to initialize the client server.
-> First, we call set_used_blocks to read in any existing
filesystem and make sure we don't make any used blocks available.
-> We initialize the client socket, bind(), then assign the port specified
(if none is specified, the OS assigns it).
-> We call listen() to await any client connections, then print the port
number.
-> When a client connects, we create a thread to handle the client's request.
-------------------------------------------------*/

int init_server(uint16_t port){
    

    std::set<uint32_t> blocks_used;

    set_used_blocks(0, blocks_used);

    for (uint32_t i = 4095; i > 0; i--) {
        if (blocks_used.find(i) == blocks_used.end()) {
            available_disk_blocks.push_back(i);
        }
    }

 


    //Set up socket clients will use
    //Make sure this works if user does not specify port number
    //Initialize Connection
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {//Failed on socket init...
    }

    int yesval = 1;
    if(setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1){
        close(tcp_socket);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    if(port == 0) {
        addr.sin_port = 0;
        if(bind(tcp_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            close(tcp_socket);
            return -1; 
        }
        // Correctly get the assigned port AFTER binding
        socklen_t addrlen = sizeof(addr);
        if (getsockname(tcp_socket, reinterpret_cast<sockaddr*>(&addr), &addrlen) == -1) {
            perror("getsockname failed");
            close(tcp_socket);
            return -1; 
        }
        server_port = ntohs(addr.sin_port); 
    }else {
        addr.sin_port = htons(port); 
        if(bind(tcp_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            close(tcp_socket);
            return -1; 
        }
        server_port = port;
    }

    
    if(listen(tcp_socket, 30) == -1) {
        
        close(tcp_socket);
    }

    print_port(server_port);
    //if accept is detected, create new boost thread to service request
    //int accept(int socket, struct sockaddr *address, int *address_len);
    //If successful, accept() returns a nonnegative. If unsuccessful, accept() returns -1 

    socklen_t addr_len = sizeof(addr);
    while(true){
        int client_socket = accept(tcp_socket, reinterpret_cast<sockaddr*>(&addr), &addr_len);

        if(client_socket > -1){
            
            boost::thread client_thread(&handle_request, client_socket);
            client_thread.detach();
        
        }else{
            break;
        }

    }



    return tcp_socket;

}

uint16_t parse_line(int argc, char *argv[]){
    //if argc == 2, port was specified
    if(argc == 1) {
        return 0;
    }else{
        return std::atoi(argv[1]);
    }
}

/*HANDLE_READBLOCK
-------------------------------------------------
-> This function is used to handle any FS_READBLOCK requests from the client.
-> This function completes most of the error checking (some being done in handle_request)
and handles the rest of the read block request.
-> In the end, if able to fetch the data requested from disk, it returns a pointer to the char buffer
containing the data read from disk, which can then be sent to the client in handle_request.
-------------------------------------------------*/

std::shared_ptr<char[]> handle_readblock(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], uint32_t block, int &status) {

    std::vector<std::string> path_vector = char_array_to_string_vector(pathname_char);
    if(path_vector.size() == 0) {
        status = -1;
        return nullptr;
    }
    std::string username = std::string(username_char);

    uint32_t child_block = 0;
    uint32_t parent_block = 0;

    int check = traverse_tree(path_vector, false, child_block, parent_block, username_char);

    //FIXME: we dont need to hold parent lock right?
    
    if(check == -1) { //Path does not exist!
       
        status = -1;
        return nullptr;
    }

    locks[parent_block]->unlock_shared();

    fs_inode node;
    char inode_buf[FS_BLOCKSIZE]; //Buffer to read inode block
    disk_readblock(child_block, inode_buf);
    memcpy(&node, inode_buf, sizeof(fs_inode)); //Copy from buffer

    if(std::strcmp(username_char, node.owner) != 0) {
        
        locks[child_block]->unlock_shared();
        
        status = -1;
        return nullptr;
    }
    
    if(node.type != 'f') { //MAKE SURE ITS ACTUALLY A FILE WERE READING FROM

        locks[child_block]->unlock_shared();

        status = -1;
        return nullptr;
    }
    

    if(block < node.size) {
    
        std::shared_ptr<char[]> buf(new char[FS_BLOCKSIZE]);
        memset(buf.get(), 0, FS_BLOCKSIZE);
        uint32_t block_read_from = node.blocks[block];
        
        disk_readblock(block_read_from, buf.get());
   
        locks[child_block]->unlock_shared();

        return buf;
    }else{ //Block is not in file!

        locks[child_block]->unlock_shared();

        status = -1;
        
        return nullptr;
    }

}

/*HANDLE_WRITEBLOCK
-------------------------------------------------
-> This function is used to handle any FS_WRITEBLOCK requests from the client.
-> This function completes most of the error checking (some being done in handle_request)
for a write block request.
-> It checks if the file is full, if the block is valid, and if the user has permission to write to the file.
-> If the file is not full, it writes the data to the block specified.
-> If the file is full, it checks if there is space on disk to write a new block.
-> If there is space, it writes the data to a new block and updates the inode.
-> If there is no space, it returns -1.
-> After a succesful write to disk, it returns 0 to let the handle_request function know that the write was successful.
-------------------------------------------------*/

int handle_writeblock(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], uint32_t block, void* data, size_t data_len) {
  
    std::vector<std::string> path_vector = char_array_to_string_vector(pathname_char);
    if(path_vector.size() == 0) {
        return -1;
    }

    std::string username = std::string(username_char);

    uint32_t child_block = 0;
    uint32_t parent_block = 0;

    int check = traverse_tree(path_vector, true, child_block, parent_block, username_char);


    if(check == -1) {    //Path does not exist!
        return -1;
    }

    locks[parent_block]->unlock_shared();

    fs_inode node;
    char inode_buf[FS_BLOCKSIZE]; // Buffer to read inode block
    disk_readblock(child_block, inode_buf);
    memcpy(&node, inode_buf, sizeof(fs_inode)); // Copy from buffer

    if(std::strcmp(username_char, node.owner) != 0) {
        
        locks[child_block]->unlock();
        
        return -1;
    }
    
    //Path is a directory!
    if(node.type != 'f') { //MAKE SURE ITS ACTUALLY A FILE WE'RE READING FROM
        
        locks[child_block]->unlock();

        return -1;
    }
    
        if(block < node.size) {

            uint32_t block_write_to = node.blocks[block];
            
            char buf[FS_BLOCKSIZE];
            memset(buf, 0, FS_BLOCKSIZE);
            memcpy(buf, data, FS_BLOCKSIZE);
            
            disk_writeblock(block_write_to, buf);
        
        }else if(block == node.size){

            //FILE IS FULL
            if(node.size == FS_MAXFILEBLOCKS) { 
        
                locks[child_block]->unlock();
                
                return -1;
            }
       
            
            ds_mutex.lock();
             //NOT ENOUGH DISK SPACE!
            if(available_disk_blocks.size() < 1) {
                ds_mutex.unlock();
                locks[child_block]->unlock();
                
                return -1;
            }

            uint32_t new_block_num = available_disk_blocks.back();
            available_disk_blocks.pop_back();
            ds_mutex.unlock();
            
            char buf[FS_BLOCKSIZE];
            memset(buf, 0, FS_BLOCKSIZE);
            memcpy(buf, data, FS_BLOCKSIZE);
            
            disk_writeblock(new_block_num, buf);
            
            //EDIT INODE AFTER DISK WRITE FOR CRASH CONSISTENCY
            node.blocks[node.size] = new_block_num;
            node.size++;
            
            char inode_buf[FS_BLOCKSIZE];
            memset(inode_buf, 0, FS_BLOCKSIZE);
            memcpy(inode_buf, &node, sizeof(fs_inode));
            disk_writeblock(child_block, inode_buf); 

        }else {
            
            locks[child_block]->unlock();
            
            return -1;
        }

        locks[child_block]->unlock();
        
    //Success!
    return 0;

}

/*HANDLE_CREATE
-------------------------------------------------
-> This function is used to handle any FS_CREATE requests from the client.
-> It calls traverse_tree_create to find and writer-lock the parent.
-> This function completes most of the error checking (some being done in handle_request).
-> It checks if the path exists, if the file already exists, and if the user has permission to create a file in the directory.
-> If everything is succesful (and blocks exist), it creates a new file or directory in the path specified.
-> This consists of making a new inode, a new direnntry slot, and even a new direntry block if necessary.
-> If any failure occurs, it returns -1, else it returns 0 to handle_request.
-------------------------------------------------*/


int handle_create(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1], char type) {
  
    
    std::vector<std::string> path_vector = char_array_to_string_vector(pathname_char);
    if(path_vector.size() == 0) {
        return -1;
    }
    std::string file_name = path_vector.back();
    std::string username = std::string(username_char);

    if(file_name.length() > FS_MAXFILENAME) {
        return -1;
    }

    
    uint32_t parent_block = 0;

    int check = 0;


    check = traverse_tree_create(path_vector, parent_block, username_char);
    if(check == -1) { //Path does not exist!
        return -1;
    }

    fs_inode node;
    char node_buf[FS_BLOCKSIZE]; //Buffer to read inode block
    
    disk_readblock(parent_block, node_buf);
    
    memcpy(&node, node_buf, sizeof(fs_inode)); //Copy from buffer

    if(node.type != 'd') {
        locks[parent_block]->unlock();
        
        return -1;
    }

    if(std::strcmp(username_char, node.owner) != 0 && parent_block != 0) {

        locks[parent_block]->unlock();

        return -1;
    }

    //ALL INODES LOCK

    bool found = false;
    uint32_t first_empty_block = 0;
    int empty_direntry_offset = -1;
    uint32_t temp_empty_block = 0;

    char temp_buf[FS_BLOCKSIZE];
    memset(temp_buf, 0, FS_BLOCKSIZE);

    char dir_block_buf[FS_BLOCKSIZE];
    memset(dir_block_buf, 0, FS_BLOCKSIZE);


    for(uint32_t i = 0; i < node.size; i++) {
        temp_empty_block = node.blocks[i];

        memset(temp_buf, 0, FS_BLOCKSIZE);
        
        disk_readblock(temp_empty_block, temp_buf);

        if(!found) {
            first_empty_block = temp_empty_block;
            memcpy(dir_block_buf, temp_buf, FS_BLOCKSIZE);
        }
        
        fs_direntry* direntries = reinterpret_cast<fs_direntry*>(temp_buf);

        for(uint32_t j = 0; j < 8; j++) {
            if(strcmp(direntries[j].name, file_name.c_str()) == 0){

                //FOUND A DUPLICATE!

                locks[parent_block]->unlock();
                
                return -1;
            }

            if(direntries[j].inode_block == 0 && !found) {
                empty_direntry_offset = j;
                found = true;
            }
        }

    }

    if(node.type != 'd') { //Can't create a file in a file!

        locks[parent_block]->unlock();

        return -1;
    }


    uint32_t temp_inode_block_num;

    if(empty_direntry_offset == -1) {//CASE WHERE LAST BLOCK IS FULL OF DIRENTRIES
      

        if(node.size == FS_MAXFILEBLOCKS) { //DIRECTORY IS FULL!
            
            locks[parent_block]->unlock();
            
            return -1;
        }

        ds_mutex.lock();

        if(available_disk_blocks.size() < 2) { //NO DISK SPACE
            ds_mutex.unlock();
            locks[parent_block]->unlock();
            
            return -1;
        }

        uint32_t new_inode_block_num = available_disk_blocks.back();
        available_disk_blocks.pop_back();
        temp_inode_block_num = new_inode_block_num;

        //TAKING THE LOWEST AVAILABLE DIRECTORY BLOCK
        uint32_t new_direntry_block_num = available_disk_blocks.back();
        available_disk_blocks.pop_back();
        
        ds_mutex.unlock();

        
        fs_direntry new_direntry;
        std::strcpy(new_direntry.name, file_name.c_str());
        new_direntry.inode_block = new_inode_block_num;

        //Handle making a new file and directory differently

        ds_mutex.lock();
        locks.emplace(temp_inode_block_num, std::make_shared<boost::shared_mutex>());
        ds_mutex.unlock();

        locks[temp_inode_block_num]->lock();

        char buf[FS_BLOCKSIZE];
        memset(buf, 0, FS_BLOCKSIZE);

        fs_inode new_inode;
        new_inode.type = type;
        std::strcpy(new_inode.owner, username_char);
        new_inode.size = 0;

        memcpy(buf, &new_inode, sizeof(fs_inode));
        
        disk_writeblock(temp_inode_block_num, buf);
                
        char dirbuf[FS_BLOCKSIZE];
        memset(dirbuf, 0, FS_BLOCKSIZE);

        memcpy(dirbuf, &new_direntry, sizeof(fs_direntry));

        
        disk_writeblock(new_direntry_block_num, dirbuf);
        
        node.blocks[node.size] = new_direntry_block_num;
        node.size++;

        char parent_buf[FS_BLOCKSIZE];
        memset(parent_buf, 0, FS_BLOCKSIZE);

        memcpy(parent_buf, &node, sizeof(fs_inode));        
        
        disk_writeblock(parent_block, parent_buf);

        
    }else{//CASE WHERE YOU CAN FIT MORE DIRENTRIES IN LAST BLOCK OF DIRECTORY
        
        ds_mutex.lock(); //LOCK AVAILABLE DISK BLOCKS

        if(available_disk_blocks.size() < 1) { //NO DISK SPACE. CHECK MUST BE INSIDE LOCK SO NO CHANGES CAN OCCUR
            
            ds_mutex.unlock();
            locks[parent_block]->unlock();
            
            return -1;
        }

        uint32_t new_inode_block_num = available_disk_blocks.back();
        available_disk_blocks.pop_back();

        ds_mutex.unlock(); //UNLOCK AVAILABLE DISK BLOCKS

        temp_inode_block_num = new_inode_block_num;

        fs_direntry new_direntry;
        std::strcpy(new_direntry.name, file_name.c_str());
        new_direntry.inode_block = new_inode_block_num;

        ds_mutex.lock();
        locks.emplace(temp_inode_block_num, std::make_shared<boost::shared_mutex>());
        ds_mutex.unlock();

        locks[temp_inode_block_num]->lock();
        
        char buf[FS_BLOCKSIZE];
        memset(buf, 0, FS_BLOCKSIZE);

        fs_inode new_inode;
        new_inode.type = type;
        std::strcpy(new_inode.owner, username_char);
        new_inode.size = 0;

        memcpy(buf, &new_inode, sizeof(fs_inode));
        
        disk_writeblock(temp_inode_block_num, buf);
    

        uint32_t offset = sizeof(fs_direntry) * empty_direntry_offset;
        
        memcpy(dir_block_buf + offset, &new_direntry, sizeof(fs_direntry));

        
        disk_writeblock(first_empty_block, dir_block_buf);

    }

    locks[temp_inode_block_num]->unlock();
    locks[parent_block]->unlock();
   
    return 0;
}

/*HANDLE_DELETE
-------------------------------------------------
-> This function is used to handle any FS_DELETE requests from the client.
-> It calls traverse_tree_delete to find and writer-lock the parent, but handles finding and writer-locking the child itself.
-> This function completes most of the error checking (some being done in handle_request)
-> It checks if the path exists, if the file exists, and if the user has permission to delete the file.
-> If everything is succesful, it deletes the file in the path specified.
-> If any failure occurs, it returns -1, else it returns 0 to handle_request.
-> Overall, if successful, it clears up space on the disk and makes blocks available as necessary,
handling requests seperately for if a direntry is the last in its disk block, or if there are more left.
-------------------------------------------------*/

int handle_delete(char username_char[FS_MAXUSERNAME + 1], char pathname_char[FS_MAXFILENAME + 1]) {
 
    std::vector<std::string> path_vector = char_array_to_string_vector(pathname_char);

    if(path_vector.size() == 0) {
        return -1;
    }

    std::string username = std::string(username_char);

    uint32_t child_block = 0;
    uint32_t parent_block = 0;


    int check = traverse_tree_delete(path_vector, child_block, parent_block, username_char);

    if(check == -1) {//Path does not exist!
        return -1;
   }


    fs_inode parent_node;
    char parent_inode_buf[FS_BLOCKSIZE]; //Buffer to read inode block
    disk_readblock(parent_block, parent_inode_buf);
    memcpy(&parent_node, parent_inode_buf, sizeof(fs_inode)); //Copy from buffer
    

    if(parent_node.type != 'd') {
        
        locks[parent_block]->unlock();
        
        return -1;
    }

    if(std::strcmp(username_char, parent_node.owner) != 0 && parent_block != 0) {
        
        locks[parent_block]->unlock();
        
        return -1;
    }
    
    uint32_t block_to_find = 0;

    uint32_t direntry_block_num = 0;
    int direntry_file_block_num = -1;
    int direntry_offset = -1;
    uint32_t direntry_block_size = 0;

    bool found = false;

    char dir_block_buf[FS_BLOCKSIZE];

    for(uint32_t i = 0; i < parent_node.size; i++) {

        direntry_block_num = parent_node.blocks[i];
        direntry_file_block_num = i;
   

        memset(dir_block_buf, 0, FS_BLOCKSIZE);
        disk_readblock(parent_node.blocks[i], dir_block_buf);
        fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

        for(uint32_t j = 0; j < 8; j++){

            if(direntries[j].inode_block != 0) {
                direntry_block_size++;
            
                if(strcmp(direntries[j].name, path_vector.back().c_str()) == 0) {
                    block_to_find = direntries[j].inode_block;
                    locks[block_to_find]->lock();
                    direntry_offset = j;
                    found = true;
                }
            }
        }

        if(found) {
            break;
        } else {
            direntry_block_size = 0;
        }
    }

    if(!found) {
      
        locks[parent_block]->unlock();

        return -1;

    } else {

        child_block = block_to_find;

    }


    fs_inode child_node;
    char inode_buf[FS_BLOCKSIZE]; 
    memset(inode_buf, 0, FS_BLOCKSIZE);
    disk_readblock(child_block, inode_buf);
    memcpy(&child_node, inode_buf, sizeof(fs_inode));

    if(std::strcmp(username_char, child_node.owner) != 0) {
        
        locks[child_block]->unlock();
        locks[parent_block]->unlock();
        
        return -1;
    }


    if(child_node.size > 0 && child_node.type == 'd') {
        
        locks[child_block]->unlock();
        locks[parent_block]->unlock();
        
        return -1;
    }
    

    if(direntry_block_size == 1) { 
              
        for(uint32_t i = direntry_file_block_num; i < parent_node.size - 1; i++) {
            parent_node.blocks[i] = parent_node.blocks[i + 1];
        }
        
        parent_node.blocks[parent_node.size - 1] = 0;
        
        parent_node.size--;

        char parent_buf[FS_BLOCKSIZE];
        memset(parent_buf, 0, FS_BLOCKSIZE);

        //EDITED PARENT BLOCK AND REMOVED DIRENTRY BLOCK AND REDUCED SIZE SO THIS MUST BE UPDATED TO DISK
        memcpy(parent_buf, &parent_node, sizeof(fs_inode));
        
        disk_writeblock(parent_block, parent_buf);

        ds_mutex.lock();
        available_disk_blocks.push_back(direntry_block_num);
        ds_mutex.unlock();
        
    } else { //CASE WHERE THERE ARE DIRENTRIES LEFT IN THE BLOCK
      
        uint32_t offset = sizeof(fs_direntry) * direntry_offset;
                
        memset(dir_block_buf + offset, 0, sizeof(fs_direntry));

        disk_writeblock(direntry_block_num, dir_block_buf);

    }

    if(child_node.type == 'f') {
        ds_mutex.lock();
        for(uint32_t i = 0; i < child_node.size; i++) {
            available_disk_blocks.push_back(child_node.blocks[i]);
        }
        ds_mutex.unlock();
        std::memset(child_node.blocks, 0, FS_MAXFILEBLOCKS * sizeof(uint32_t));

        child_node.size = 0;
    }

    ds_mutex.lock();
    locks.erase(child_block);
    available_disk_blocks.push_back(child_block);
    ds_mutex.unlock();

    locks[parent_block]->unlock();
    return 0;
}


/*CHAR_ARRAY_TO_STRING_VECTOR
------------------------------------------------
-> Helper function used to parse the pathname provided by a user
and determine its validity.
-> For example, if the pathname is empty, starts with a '/', ends with
a '/', or has multiple slashes in a row, the pathname is invalid.
------------------------------------------------*/

std::vector<std::string> char_array_to_string_vector(char char_array[FS_MAXFILENAME + 1]) {
    std::vector<std::string> string_vector;
    std::string str(char_array);
    std::istringstream iss(str);
    std::string token;

        if (str.empty()) {
            return {};
        }
    
        if(str == "/") {
            return {};
        }

        //Check if there's at least one '/'
        if (str.find('/') == std::string::npos) {
            return {};
        }

        //Check if first char is a forward slash
        if(str[0] != '/'){
            return {};
        }

        //Check if the last char is a slash
        if(str.back() == '/'){
            return {};
        }

    bool is_first_slash = true;
    while (std::getline(iss, token, '/')) {
        if(!token.empty()){ //Was pushing in an empty string
            string_vector.push_back(token);
        }else{
            if(is_first_slash) {
                is_first_slash = false;
            }else {
                return {};
            }
        }
    }

    return string_vector;
}


/*TRAVERSE_TREE_DELETE
-------------------------------------------------
-> This function is used by handle_delete to traverse the file system.
-> It takes advantage of hand-over-hand locking in order to make sure that
the parent is writer-locked before returning back to handle_delete.
-> It goes through the file system by reading in the inode blocks, reading its direntries,
and looking for the next file/directory in the path until finding the parent of the file/directory.
-> The parent block is passed in by reference so the parent can be updated in handle_delete.
-------------------------------------------------*/

int traverse_tree_delete(std::vector<std::string> path_vector, uint32_t& child_block, uint32_t& parent_block, char user[FS_MAXUSERNAME + 1]) {
    std::string name_to_find = path_vector.back();
    uint32_t block_to_find = 0;

    bool found = false;

    uint32_t current_block = 0;
    
    if(path_vector.size() == 1) {
        locks[current_block]->lock();
    }else {
        locks[current_block]->lock_shared();
    }
    

    fs_inode main_inode;

    if(path_vector.size() > 1) {
        path_vector.pop_back();

        std::string parent_name = path_vector.back();


        for(uint32_t i = 0; i < path_vector.size(); i++) {
            char main_inode_buf[FS_BLOCKSIZE]; //Buffer to read inode block
            
            disk_readblock(current_block, main_inode_buf);
            
            memcpy(&main_inode, main_inode_buf, sizeof(fs_inode)); //Copy from buffer

            if(main_inode.type != 'd') {
                
                locks[current_block]->unlock_shared();
                
                return -1;
            }

            if(std::strcmp(user, main_inode.owner) != 0 && current_block != 0) {
        
                locks[current_block]->unlock_shared();
        
                return -1;
            }

            for(uint32_t j = 0; j < main_inode.size; j++) {

                char dir_block_buf[FS_BLOCKSIZE];
                memset(dir_block_buf, 0, FS_BLOCKSIZE);
                
                disk_readblock(main_inode.blocks[j], dir_block_buf);
                
                fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

                for(uint32_t k = 0; k < 8; k++){
                    if(direntries[k].inode_block != 0){
                    
                        if(strcmp(direntries[k].name, path_vector[i].c_str()) == 0) {
                            block_to_find = direntries[k].inode_block;
                            found = true;
                            
                            if(i == path_vector.size() - 1 ) {
                                locks[block_to_find]->lock();
                            }else {
                                locks[block_to_find]->lock_shared();
                            }

                            locks[current_block]->unlock_shared();
                            
                            break;
                        }

                    }
                }

                if(found) {
                    break;
                }

            }

            if(!found) {
                
                locks[current_block]->unlock_shared();
    
                return -1;
            } else {

                current_block = block_to_find;
                found = false;

            }
        }
    }
   
    parent_block = current_block;


    return 0;
}



/*TRAVERSE_TREE
--------------------------------------------------------------
-> Used by handle_readblock and handle_writeblock to traverse the 
filesystem and locate the parent and child blocks.
-> If the path doesn't exist, traverse tree will unlock the current_block's
lock and return -1.
-> Traverse_tree utilizes hand-over-hand locking when traversing using shared
and regular locks. For example, traverse tree will lock the parent, lock the child
while locking the parent, then release the parent as it traverses down.
-> If the request is a write operation, traverse_tree will lock the child with
a writer lock to protect that critical section.
--------------------------------------------------------------*/

int traverse_tree(std::vector<std::string> path_vector, bool write_child, uint32_t& child_block, uint32_t& parent_block, char user[FS_MAXUSERNAME + 1]) {
    std::string name_to_find = path_vector.back();
    uint32_t block_to_find = 0;

    bool found = false;

    uint32_t current_block = 0;
    
    locks[current_block]->lock_shared();
    

    fs_inode main_inode;

    if(path_vector.size() > 1) {
        path_vector.pop_back();

        std::string parent_name = path_vector.back();


        for(uint32_t i = 0; i < path_vector.size(); i++) {
            char main_inode_buf[FS_BLOCKSIZE]; // Buffer to read inode block
            
            disk_readblock(current_block, main_inode_buf);
            
            memcpy(&main_inode, main_inode_buf, sizeof(fs_inode)); // Copy from buffer

            if(main_inode.type != 'd') {
                
                locks[current_block]->unlock_shared();
                
                return -1;
            }

            if(std::strcmp(user, main_inode.owner) != 0 && current_block != 0) {
        
                locks[current_block]->unlock_shared();
        
                return -1;
            }

            for(uint32_t j = 0; j < main_inode.size; j++) {

                char dir_block_buf[FS_BLOCKSIZE];
                memset(dir_block_buf, 0, FS_BLOCKSIZE);
                
                disk_readblock(main_inode.blocks[j], dir_block_buf);
                
                fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

                for(uint32_t k = 0; k < 8; k++){
                    if(direntries[k].inode_block != 0){
                    
                        if(strcmp(direntries[k].name, path_vector[i].c_str()) == 0) {
                            block_to_find = direntries[k].inode_block;
                            found = true;
                            
                            locks[block_to_find]->lock_shared();

                            locks[current_block]->unlock_shared();
                            
                            break;
                        }

                    }
                }

                if(found) {
                    break;
                }

            }

            if(!found) {
                
                locks[current_block]->unlock_shared();
    
                return -1;
            } else {

                current_block = block_to_find;
                found = false;

            }
        }
    }
   
    parent_block = current_block;

    fs_inode parent_inode;
    char parent_inode_buf[FS_BLOCKSIZE]; // Buffer to read inode block
    
    disk_readblock(parent_block, parent_inode_buf);
    
    memcpy(&parent_inode, parent_inode_buf, sizeof(fs_inode)); // Copy from buffer

    if(parent_inode.type != 'd') {
        
        locks[parent_block]->unlock_shared();
        
        return -1;
    }

    if(std::strcmp(user, parent_inode.owner) != 0 && parent_block != 0) {
        
        locks[parent_block]->unlock_shared();

        return -1;
    }

    for(uint32_t i = 0; i < parent_inode.size; i++) {
   
        char dir_block_buf[FS_BLOCKSIZE];
        memset(dir_block_buf, 0, FS_BLOCKSIZE);
        
        disk_readblock(parent_inode.blocks[i], dir_block_buf);
        
        fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

        for(uint32_t j = 0; j < 8; j++){
            if(direntries[j].inode_block != 0){
               
                if(strcmp(direntries[j].name, name_to_find.c_str()) == 0) {
                  
                    block_to_find = direntries[j].inode_block;
                    found = true;
                    
                    if(write_child) {
                        locks[block_to_find]->lock();
                    } else{
                        locks[block_to_find]->lock_shared();
                    }
                    
                    break;
                }
            }
        }

        if(found) {
            break;
        }
    }

    if(!found) {
      
        locks[current_block]->unlock_shared();
      
        return -1;
    } else {

        child_block = block_to_find;
        found = false;

    }

    return 0;
}

/*TRAVERSE_TREE_CREATE
-------------------------------------------------
-> This function is used by handle_create to traverse the file system.
-> It takes advantage of hand-over-hand locking in order to make sure that
the parent is writer-locked before returning back to handle_create.
-> It goes through the file system by reading in the inode blocks, reading its direntries,
and looking for the next file/directory in the path until finding the parent of the file/directory.
-> The parent block is passed in by reference so the parent can be updated in handle_create.
-------------------------------------------------*/

int traverse_tree_create(std::vector<std::string> path_vector, uint32_t& parent_block, char user[FS_MAXUSERNAME + 1]) {
    std::string file_name = path_vector.back();

    uint32_t block_to_find = 0;

    bool found = false;

    uint32_t current_block = 0;
    
    if(path_vector.size() == 1) {
        locks[current_block]->lock();
    }else {
        locks[current_block]->lock_shared();
    }

    fs_inode main_inode;

    if(path_vector.size() > 1) {
        path_vector.pop_back();

        std::string parent_name = path_vector.back();

        for(uint32_t i = 0; i < path_vector.size(); i++) {
            char main_inode_buf[FS_BLOCKSIZE]; // Buffer to read inode block
            
            disk_readblock(current_block, main_inode_buf);
            
            memcpy(&main_inode, main_inode_buf, sizeof(fs_inode)); // Copy from buffer

            if(main_inode.type != 'd') {
                
                locks[current_block]->unlock_shared();
                
                return -1;
            }  

            if(std::strcmp(user, main_inode.owner) != 0 && current_block != 0) {
        
                locks[current_block]->unlock_shared();
        
                return -1;
            }



            for(uint32_t j = 0; j < main_inode.size; j++) {

                char dir_block_buf[FS_BLOCKSIZE];
                memset(dir_block_buf, 0, FS_BLOCKSIZE);
                
                disk_readblock(main_inode.blocks[j], dir_block_buf);
                
                fs_direntry* direntries = reinterpret_cast<fs_direntry*>(dir_block_buf);

                for(uint32_t k = 0; k < 8; k++){
                    if(direntries[k].inode_block != 0){
                    
                        if(strcmp(direntries[k].name, path_vector[i].c_str()) == 0) {
                            block_to_find = direntries[k].inode_block;
                            found = true;

                            if(i == path_vector.size() - 1 ) {
                                locks[block_to_find]->lock();
                            }else {
                                locks[block_to_find]->lock_shared();
                            }
                            locks[current_block]->unlock_shared();
                            
                            break;
                        }

                    }
                }

                if(found) {
                    break;
                }

            }

            if(!found) {
                
                locks[current_block]->unlock_shared();
    
                return -1;
            } else {

                current_block = block_to_find;
                found = false;

            }
        }
    }
   
    parent_block = current_block;

    return 0;
}
