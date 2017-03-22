#include <stdio.h>
#include <stdlib.h>
#include "TinyFrame.h"

#define BUFLEN 200

bool listen(unsigned int frame_id, const unsigned char *buff, unsigned int len)
{
	printf("\033[33mrx frame %s, len %d, id %d\033[0m\n", buff, len, frame_id);

	return false; // Do not unbind
}

void main()
{
	TF_Init(1);

	int msgid;
	unsigned char buff[BUFLEN];

	TF_AddListener(TF_ANY_ID, listen);

	int len = TF_Compose(buff, &msgid, "Hello TinyFrame", 0, TF_NEXT_ID);
	printf("Used = %d, id=%d\n",len, msgid);
	for(int i=0;i<len; i++) {
		printf("%3u %c\n", buff[i], buff[i]);
	}
	TF_Accept(buff, len);

	len = TF_Compose(buff, &msgid, "PRASE PRASE", 5, TF_NEXT_ID);
	printf("Used = %d, id=%d\n",len, msgid);
	for(int i=0;i<len; i++) {
		printf("%3u %c\n", buff[i], buff[i]);
	}
	TF_Accept(buff, len);
}
