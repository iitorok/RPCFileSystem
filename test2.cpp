#include <iostream>
#include <cassert>
#include <cstdlib>
#include "fs_client.h"

int main(int argc, char* argv[]) {
    //Test reader and writer lock usage
    char* server;
    int server_port;

    const char* writes = "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss";

    char readdata[FS_BLOCKSIZE];
    int status = -2;

    if (argc != 3) {
        std::cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    
    //Create some directories

    status = fs_create("user1", "/dir", 'd');
    
    if(!status){
        std::cout<<"SUCCESS! line 28"<<std::endl;
    }else{
        std::cout<<"FAIL! line 30"<<std::endl;
    }

    fs_create("user1", "/dir/file", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 35"<<std::endl;
    }else{
        std::cout<<"FAIL! line 37"<<std::endl;
    }
    
    fs_create("user1", "/dir/file2", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 42"<<std::endl;
    }else{
        std::cout<<"FAIL! line 44"<<std::endl;
    }
    
    fs_create("user1", "/dir/home", 'd');

    if(!status){
        std::cout<<"SUCCESS! line 50"<<std::endl;
    }else{
        std::cout<<"FAIL! line 52"<<std::endl;
    }
    
    fs_create("user1", "/dir/home/photo", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 57"<<std::endl;
    }else{
        std::cout<<"FAIL! line 59"<<std::endl;
    }

    fs_create("user1", "/dir/home/snapshot", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 63"<<std::endl;
    }else{
        std::cout<<"FAIL! line 65"<<std::endl;
    }

    fs_writeblock("user1", "/dir/home/photo", 0, writes);
        if(!status){
        std::cout<<"SUCCESS! line 69"<<std::endl;
    }else{
        std::cout<<"FAIL! line 71"<<std::endl;
    }

    fs_writeblock("user1", "/dir/home/snapshot", 3, writes);
    if(!status){
        std::cout<<"SUCCESS! line 75"<<std::endl;
    }else{
        std::cout<<"FAIL! line 77"<<std::endl;
    }

   fs_readblock("user1", "/dir/home/snapshot", 3, readdata);
   if(!status){
    std::cout<<"SUCCESS! line 82"<<std::endl;
    }else{
    std::cout<<"FAIL! line 84"<<std::endl;
    }

    fs_create("user2", "/dir2", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 89"<<std::endl;
    }else{
        std::cout<<"FAIL! line 91"<<std::endl;
    }   

    //ERROR!
    fs_create("user2", "/dir2/here/newdir", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 97"<<std::endl;
    }else{
        std::cout<<"FAIL! line 99"<<std::endl;
    }
    //ERROR!
    fs_writeblock("user2", "/dir2/here/newdir", 0, writes);
    if(!status){
        std::cout<<"SUCCESS! line 103"<<std::endl;
    }else{
        std::cout<<"FAIL! line 106"<<std::endl;
    }

    fs_create("user2", "/dir2/here", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 111"<<std::endl;
    }else{
        std::cout<<"FAIL! line 113"<<std::endl;
    }
    fs_create("user2", "/dir2/documents", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 117"<<std::endl;
    }else{
        std::cout<<"FAIL! line 119"<<std::endl;
    }
    //ERROR! Duplicate!
   // fs_create("user2", "/dir2/documents/documents", 'd');
    fs_create("user2", "/dir2/documents/docfile", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 125"<<std::endl;
    }else{
        std::cout<<"FAIL! line 127"<<std::endl;
    }

    fs_writeblock("user2", "/dir2/documents/docfile", 3, writes);
    if(!status){
        std::cout<<"SUCCESS! line 132"<<std::endl;
    }else{
        std::cout<<"FAIL! line 134"<<std::endl;
    }

    fs_readblock("user2", "/dir2/documents/docfile", 3, readdata);
    if(!status){
        std::cout<<"SUCCESS! line 139"<<std::endl;
    }else{
        std::cout<<"FAIL! line 141"<<std::endl;
    }
    //allocate a lot of directories
    fs_create("user1", "/dir/d1", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 146"<<std::endl;
    }else{
        std::cout<<"FAIL! line 148"<<std::endl;
    }
    fs_create("user1", "/dir/d2", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 152"<<std::endl;
    }else{
        std::cout<<"FAIL! line 154"<<std::endl;
    }
    fs_create("user1", "/dir/d3", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 158"<<std::endl;
    }else{
        std::cout<<"FAIL! line 160"<<std::endl;
    }
    fs_create("user1", "/dir/d4", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 164"<<std::endl;
    }else{
        std::cout<<"FAIL! line 166"<<std::endl;
    }
    fs_create("user1", "/dir/d5", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 170"<<std::endl;
    }else{
        std::cout<<"FAIL! line 172"<<std::endl;
    }
    fs_create("user1", "/dir/d6", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 176"<<std::endl;
    }else{
        std::cout<<"FAIL! line 178"<<std::endl;
    }
    fs_create("user1", "/dir/d7", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 182"<<std::endl;
    }else{
        std::cout<<"FAIL! line 184"<<std::endl;
    }
    
    //delete some
    fs_delete("user1", "/dir/d1");
    if(!status){
        std::cout<<"SUCCESS! line 190"<<std::endl;
    }else{
        std::cout<<"FAIL! line 192"<<std::endl;
    }
    
    fs_delete("user1", "/dir/d5");
    if(!status){
        std::cout<<"SUCCESS! line 196"<<std::endl;
    }else{
        std::cout<<"FAIL! line 198"<<std::endl;
    }

    fs_delete("user1", "/dir/d7");
    if(!status){
        std::cout<<"SUCCESS! line 202"<<std::endl;
    }else{
        std::cout<<"FAIL! line 204"<<std::endl;
    }

    //make sure new ones are being placed at the lowest numbers
    fs_create("user1", "/dir/nd1", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 210"<<std::endl;
    }else{
        std::cout<<"FAIL! line 212"<<std::endl;
    }
    fs_create("user1", "/dir/nd2", 'd');
    if(!status){
        std::cout<<"SUCCESS! line 216"<<std::endl;
    }else{
        std::cout<<"FAIL! line 218"<<std::endl;
    }
    fs_create("user1", "/dir/f3", 'f');
    if(!status){
        std::cout<<"SUCCESS! line 222"<<std::endl;
    }else{
        std::cout<<"FAIL! line 224"<<std::endl;
    }

}