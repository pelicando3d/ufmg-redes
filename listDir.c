#include <iostream>
#include <cstdlib>
#include <dirent.h>
#include <string.h>

using std::cin;
using std::cout;
using std::string;

int main(){
	string dirname = "./";	
    DIR *pd = 0;
    struct dirent *pdirent = 0;
 	pd = opendir(dirname.c_str());

    if(pd == NULL)
    {
        cout << "ERROR: Please provide a valid directory path.\n";
        return -22;
    }


    while (pdirent != NULL){
 		pdirent=readdir(pd);
 		cout << pdirent->d_name << "\n";
	}
	closedir(pd);

    return 0;
}