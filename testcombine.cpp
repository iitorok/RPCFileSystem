#include <iostream>
#include <cassert>
#include <cstdlib>
#include "fs_client.h"

int main(int argc, char* argv[]) {
    //Test reader and writer lock usage
    char* server;
    int server_port;

    const char* writes = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    
    char readdata[FS_BLOCKSIZE];

    
    int status = -2;

    if (argc != 3) {
        std::cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    
    fs_create("mason", "/dictionary", 'd');
    fs_writeblock("tom", "/documents/GitHub/repos/ai", 0, writes);

    fs_create("tom", "/documents/GitHub/unreal/fungame", 'f');
    fs_writeblock("tom", "/documents/GitHub/repos/ai", 0, writes);
    
    fs_delete("tom", "/documents/GitHub/repos/ai");


    fs_readblock("tom", "/documents/GitHub/repos/ai", 0, readdata);

    fs_create("tom", "/documents/GitHub/unreal/aaaaaa", 'f');
    fs_create("tom", "/documents/GitHub/unreal/bbbbb", 'f');
    fs_create("tom", "/documents/GitHub/unreal/ggggg", 'f');
    fs_create("tom", "/documents/GitHub/unreal/ccccc", 'f');
    fs_create("tom", "/documents/GitHub/unreal/ddddd", 'f');
    fs_create("tom", "/documents/GitHub/unreal/eeeee", 'f');
    fs_create("tom", "/documents/GitHub/unreal/fffff", 'f');
    fs_create("tom", "/documents/GitHub/unreal/ggggg", 'f');

    fs_delete("tom", "/documents/GitHub/unreal/bbbbb");
    fs_delete("tom", "/documents/GitHub/unreal/ddddd");
    fs_delete("tom", "/documents/GitHub/unreal/eeeee");

    fs_create("tom", "/documents/GitHub/unreal/aabb", 'f');

    fs_create("samanthasamanthasamantha", "/documents/GitHub/unreal/aabb", 'f');

    fs_create("tom", "/documents/GitHub/3gonooenrgnegonwergnewgnweognweporgnweporgnwergnweropgnwerpognwerpgnqegpneqbpoenrgpoqwngpeqgnpeqworngweporhnqeponwerponewpognerpognewropgnwerhpnewrhpowernhoerngperognwerognewrognwerpgnwerpohnwerohnerwergbeibgebgerbgekrbgdjlbglriabgouiebsgbesrgkbesrkjgberigbeirugbdfbgserbgiurbgiudsbfgusbergibdfsbdrgbriubgdfbgirubsgiubdsglkbsdrlgbdslgrb", 'f');

    fs_create("j", "/iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", 'f');

    fs_create("tom", "/dictionary/hi", 'f');

    fs_create("tom", "/documents/GitHub/unreal/newunrealdir", 'd');

    fs_create("tom", "/documents/GitHub/unreal/newunrealdir/file", 'd');

    fs_writeblock("tom", "/documents/GitHub/unreal/newunrealdir/file", 0, writes);
    fs_writeblock("tom", "/documents/GitHub/unreal/newunrealdir/file", 1, writes);
    fs_writeblock("tom", "/documents/GitHub/unreal/newunrealdir/file", 2, writes);

}