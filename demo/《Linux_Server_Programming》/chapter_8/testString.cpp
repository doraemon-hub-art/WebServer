#include <stdio.h>
#include <string.h>

void test( char* szTemp){
    char* szURL = strpbrk( szTemp, " \t" );

    *szURL++ = '\0';// 将\t用\0覆盖，然后++指针。指向后面的内容

    char* szMethod = szTemp;// 保存请求的方法,到\0会被截断。
    printf("%s\n",szMethod);
}
int main(void){
    char * c = (char*)"GET /index.html HTTP/1.1";
    test(c);
    return 0;
}


/*

    为什么本代码中的就可以修改，因为szTemp是指向buffer的！
    
*/