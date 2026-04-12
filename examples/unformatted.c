// unformatted.c — example for cindent
//
// Deliberately messy formatting to demonstrate cindent.
//
// Run: cindent examples/unformatted.c
// Run: cindent -g examples/unformatted.c   # GNU indent backend

#include <stdio.h>
#include <stdlib.h>

int compute(int x,int y,int z){
if(x>0)
return y+z;
else
return y-z;}

void loop_example(int n)
{
for(int i=0;i<n;i++){printf("%d\n",i);}
}

typedef struct{int x;int y;}Point;

int main(void){
int result=compute(1,2,3);
if(result>0)printf("positive\n");
loop_example(5);
return 0;
}
