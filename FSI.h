#ifndef FSI_H
#define FSI_H

#include <inttypes.h>

#pragma pack(push)
#pragma pack(1)
struct fsi_struct {
	unsigned long FSI_LeadSig;	
	unsigned char FSI_Reserved1[480];	
	unsigned long FSI_StrucSig;		
	unsigned long FSI_Free_Count;		
	unsigned long FSI_Nxt_Free;		
	unsigned char FSI_Reserved2[12];	
	unsigned long FSI_TrailSig; 
};
#pragma pack(pop)

typedef struct fsi_struct FSI;

#endif