/*
 * =====================================================================================
 *
 *       Filename:  testfunc.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/23/2014 12:46:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>

int funcio(int b, int (*f)(int)){
    b = 2;
    return f(2);
}
int funcio2(int b){
    b = 3;
    return b;
}

int main(void){
    int i;
    int (*func)(int a, int(*f)(int));
    
    func = funcio;
    i = 1;

    printf("Res : %d\n", func(2, funcio2));
    return 0;
}
